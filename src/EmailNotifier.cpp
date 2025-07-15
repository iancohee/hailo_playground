#include "EmailNotifier.hpp"

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <cassert>
#include <cstring>
#include <memory>


struct ReadData
{
    const char *message;
    size_t offset;
};

struct ReadImageData
{
    const uint8_t* data;
    size_t size;
    size_t offset;
};

size_t
EmailNotifier::sendReadData (
    char* buffer,
    size_t size,
    size_t nitems,
    void* userdata
)
{
    assert(size == 1);
    assert(buffer != nullptr);
    assert(userdata != nullptr);

    ReadData* readData = (ReadData*)userdata;
    const char* data = readData->message + readData->offset;

    size_t room = size * nitems;
    size_t messageLen = strlen(data);
    if (messageLen > room)
        readData->offset = room;
    else
        readData->offset = messageLen;

    if (messageLen)
    {
        memcpy(buffer, data, readData->offset);
        return readData->offset;
    }
    return 0;    
}

size_t
EmailNotifier::sendReadImageData (
    char* ptr,
    size_t size,
    size_t nitems,
    void* userp
)
{
    assert(size == 1);
    assert(ptr != nullptr);
    assert(userp != nullptr);

    ReadImageData* imageData = static_cast<ReadImageData*>(userp);

    size_t max = size * nitems;
    size_t remaining = imageData->size - imageData->offset;
    size_t toCopy = (remaining > max) ? max : remaining;
    if (toCopy > 0)
    {
        memcpy(ptr, imageData->data, toCopy);
        imageData->offset += toCopy;
        return imageData->offset;
    }
    return 0;
}

EmailNotifier::EmailNotifier (
    const std::string& username,
    const std::string& password,
    const std::string& url
)
:
    m_username(username),
    m_password(password),
    m_url(url)
{ }

EmailCode
EmailNotifier::connectAndSendImage (
    const std::string& from,
    const std::vector<std::string>& to,
    const std::string& body,
    const std::vector<uint8_t>& imageData
)
{
    CURL* curl;
    curl = curl_easy_init();
    if (!curl)
        return CURLE_FAILED_INIT;

#ifdef DBG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif
    
    // auth
    curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERNAME, m_username.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, m_password.c_str());

    // force ssl
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);

    // mail from
    std::string toStr("<" + from + ">");
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, toStr.c_str());

    // mail to
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT_ALLLOWFAILS, 1L);
    std::vector<std::string> formattedRecipients;
    for (const auto& recipient : to)
    {
        std::string tmp = "<" + recipient + ">";
        formattedRecipients.push_back(tmp);
    }

    struct curl_slist* rcpt = nullptr;
    for (const auto& recipient : formattedRecipients)
    {
        rcpt = curl_slist_append(rcpt, recipient.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, rcpt); 
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Subject: [ai detection]");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Start of MIME parts
    curl_mime *mime = curl_mime_init(curl);
    if (!mime)
        return CURLE_OUT_OF_MEMORY;

    // body (MIME part)
    curl_mimepart *part = curl_mime_addpart(mime);
    curl_mime_data(part, body.c_str(), CURL_ZERO_TERMINATED);
    curl_mime_type(part, "text/plain");

    // attach image (MIME part)
    part = curl_mime_addpart(mime);
    if (!part)
        return CURLE_OUT_OF_MEMORY;

    ReadImageData jpgData = {
        .data = imageData.data(),
        .size = imageData.size(),
        .offset = 0
    };

    CURLcode status = CURLE_OK;
    status = curl_mime_data(
        part,
        reinterpret_cast<const char*>(jpgData.data),
        jpgData.size);
    if (status != CURLE_OK)
    {
        std::cerr << "EmailNotifier add mime data cb: " << std::endl;
        goto free;
    }
    curl_mime_filename(part, "image.jpg");
    curl_mime_type(part, "image/jpeg");
    curl_mime_encoder(part, "base64");
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    status = curl_easy_perform(curl);
    if (status != CURLE_OK)
    {
        std::cerr << "EmailNotifier connect and perform: " << std::endl;
        goto free;
    }

free:
    curl_slist_free_all(rcpt);
    curl_slist_free_all(headers);
    curl_mime_free(mime);
    curl_easy_cleanup(curl);

    return status;
}
