#ifndef EMAIL_NOTIFIER_H
#define EMAIL_NOTIFIER_H

#include <curl/curl.h>

#include <string>
#include <vector>

using EmailCode = CURLcode;

class EmailNotifier
{
public:
    EmailNotifier (
        const std::string& username,
        const std::string& password,
        const std::string& url);

    ~EmailNotifier () = default;

    EmailCode
    connectAndSendImage (
        const std::string& mailFrom,
        const std::vector<std::string>& recipients,
        const std::string& body,
        const std::vector<uint8_t>& imageData);

private:
    const std::string m_username;
    const std::string m_password;
    const std::string m_url;

    size_t
    sendReadData (char* buffer,
        size_t size,
        size_t nitems,
        void* userdata);

    size_t
    sendReadImageData (char* ptr,
        size_t size,
        size_t nitems,
        void* userp);
};

#endif // EMAIL_NOTIFIER_H