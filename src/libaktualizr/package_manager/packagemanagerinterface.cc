#include "packagemanagerinterface.h"

#include "http/httpclient.h"
#include "logging/logging.h"

struct DownloadMetaStruct {
  DownloadMetaStruct(Uptane::Target target_in, FetcherProgressCb progress_cb_in, const api::FlowControlToken* token_in)
      : hash_type{target_in.hashes()[0].type()},
        target{std::move(target_in)},
        token{token_in},
        progress_cb{std::move(progress_cb_in)} {}
  uint64_t downloaded_length{0};
  unsigned int last_progress{0};
  std::unique_ptr<StorageTargetWHandle> fhandle;
  const Uptane::Hash::Type hash_type;
  MultiPartHasher& hasher() {
    switch (hash_type) {
      case Uptane::Hash::Type::kSha256:
        return sha256_hasher;
      case Uptane::Hash::Type::kSha512:
        return sha512_hasher;
      default:
        throw std::runtime_error("Unknown hash algorithm");
    }
  }
  Uptane::Target target;
  const api::FlowControlToken* token;
  FetcherProgressCb progress_cb;

 private:
  MultiPartSHA256Hasher sha256_hasher;
  MultiPartSHA512Hasher sha512_hasher;
};

static size_t DownloadHandler(char* contents, size_t size, size_t nmemb, void* userp) {
  assert(userp);
  auto ds = static_cast<DownloadMetaStruct*>(userp);
  uint64_t downloaded = size * nmemb;
  uint64_t expected = ds->target.length();
  if ((ds->downloaded_length + downloaded) > expected) {
    return downloaded + 1;  // curl will abort if return unexpected size;
  }

  // incomplete writes will stop the download (written_size != nmemb*size)
  size_t written_size = ds->fhandle->wfeed(reinterpret_cast<uint8_t*>(contents), downloaded);
  ds->hasher().update(reinterpret_cast<const unsigned char*>(contents), written_size);

  ds->downloaded_length += downloaded;
  return written_size;
}

static int ProgressHandler(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
  (void)dltotal;
  (void)dlnow;
  (void)ultotal;
  (void)ulnow;
  auto ds = static_cast<DownloadMetaStruct*>(clientp);

  uint64_t expected = ds->target.length();
  auto progress = static_cast<unsigned int>((ds->downloaded_length * 100) / expected);
  if (ds->progress_cb && progress > ds->last_progress) {
    ds->last_progress = progress;
    ds->progress_cb(ds->target, "Downloading", progress);
  }
  if (ds->token != nullptr && !ds->token->canContinue(false)) {
    return 1;
  }
  return 0;
}

static void restoreHasherState(MultiPartHasher& hasher, StorageTargetRHandle* data) {
  size_t data_len;
  size_t buf_len = 1024;
  uint8_t buf[buf_len];
  do {
    data_len = data->rread(buf, buf_len);
    hasher.update(buf, data_len);
  } while (data_len != 0);
}

bool PackageManagerInterface::fetchTarget(const Uptane::Target& target, Uptane::Fetcher& fetcher,
                                          const KeyManager& keys, FetcherProgressCb progress_cb,
                                          const api::FlowControlToken* token) {
  (void)keys;
  bool result = false;
  try {
    if (target.hashes().empty()) {
      throw Uptane::Exception("image", "No hash defined for the target");
    }
    TargetStatus exists = PackageManagerInterface::verifyTarget(target);
    if (exists == TargetStatus::kGood) {
      LOG_INFO << "Image already downloaded; skipping download";
      return true;
    }
    if (target.length() == 0) {
      LOG_WARNING << "Skipping download of target with length 0";
      return true;
    }
    std::unique_ptr<DownloadMetaStruct> ds = std_::make_unique<DownloadMetaStruct>(target, progress_cb, token);
    if (exists == TargetStatus::kIncomplete) {
      auto target_check = storage_->checkTargetFile(target);
      ds->downloaded_length = target_check->first;
      auto target_handle = storage_->openTargetFile(target);
      ::restoreHasherState(ds->hasher(), target_handle.get());
      target_handle->rclose();
      ds->fhandle = target_handle->toWriteHandle();
    } else {
      // If the target was found, but is oversized or the hash doesn't match,
      // just start over.
      ds->fhandle = storage_->allocateTargetFile(false, target);
    }

    std::string target_url = target.uri();
    if (target_url.empty()) {
      target_url = fetcher.getRepoServer() + "/targets/" + Utils::urlEncode(target.filename());
    }

    HttpResponse response;
    for (;;) {
      response = http_->download(target_url, DownloadHandler, ProgressHandler, ds.get(),
                                 static_cast<curl_off_t>(ds->downloaded_length));

      if (response.curl_code == CURLE_RANGE_ERROR) {
        LOG_WARNING << "The image server doesn't support byte range requests,"
                       " try to download the image from the beginning: "
                    << target_url;
        ds = std_::make_unique<DownloadMetaStruct>(target, progress_cb, token);
        ds->fhandle = storage_->allocateTargetFile(false, target);
        continue;
      }

      if (!response.wasInterrupted()) {
        break;
      }
      ds->fhandle.reset();
      // sleep if paused or abort the download
      if (!token->canContinue()) {
        throw Uptane::Exception("image", "Download of a target was aborted");
      }
      ds->fhandle = storage_->openTargetFile(target)->toWriteHandle();
    }
    LOG_TRACE << "Download status: " << response.getStatusStr() << std::endl;
    if (!response.isOk()) {
      if (response.curl_code == CURLE_WRITE_ERROR) {
        throw Uptane::OversizedTarget(target.filename());
      }
      throw Uptane::Exception("image", "Could not download file, error: " + response.error_message);
    }
    if (!target.MatchHash(Uptane::Hash(ds->hash_type, ds->hasher().getHexDigest()))) {
      ds->fhandle->wabort();
      throw Uptane::TargetHashMismatch(target.filename());
    }
    ds->fhandle->wcommit();
    result = true;
  } catch (const Uptane::Exception& e) {
    LOG_WARNING << "Error while downloading a target: " << e.what();
  }
  return result;
}

TargetStatus PackageManagerInterface::verifyTarget(const Uptane::Target& target) const {
  auto target_exists = storage_->checkTargetFile(target);
  if (!target_exists) {
    return TargetStatus::kNotFound;
  } else if (target_exists->first < target.length()) {
    return TargetStatus::kIncomplete;
  } else if (target_exists->first > target.length()) {
    return TargetStatus::kOversized;
  }

  // Even if the file exists and the length matches, recheck the hash.
  DownloadMetaStruct ds(target, nullptr, nullptr);
  ds.downloaded_length = target_exists->first;
  auto target_handle = storage_->openTargetFile(target);
  ::restoreHasherState(ds.hasher(), target_handle.get());
  target_handle->rclose();
  if (!target.MatchHash(Uptane::Hash(ds.hash_type, ds.hasher().getHexDigest()))) {
    LOG_ERROR << "Target exists with expected length, but hash does not match metadata! " << target;
    return TargetStatus::kHashMismatch;
  }

  return TargetStatus::kGood;
}
