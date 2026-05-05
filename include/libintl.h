#pragma once

#include <string_view>

#if defined(_WIN32) && defined(LIBINTL_LITE_EXPORTS)
#define LIBINTL_LITE_API __declspec(dllexport)
#else
#define LIBINTL_LITE_API
#endif

extern "C" {

LIBINTL_LITE_API bool LoadMessageCatalog(std::string_view domain, std::string_view moFilePath);
LIBINTL_LITE_API char* bindtextdomain(const char* domain, const char* root);
LIBINTL_LITE_API char* textdomain(const char* domain);

LIBINTL_LITE_API char* gettext(const char* str);
LIBINTL_LITE_API char* dgettext(const char* domain, const char* str);
LIBINTL_LITE_API char* ngettext(const char* str, const char* plural, unsigned long n);
LIBINTL_LITE_API char* dngettext(const char* domain, const char* str, const char* plural, unsigned long n);
LIBINTL_LITE_API const char* pgettext(const char* context, const char* str);
LIBINTL_LITE_API const char* npgettext(const char* context, const char* str, const char* plural, unsigned long n);

}