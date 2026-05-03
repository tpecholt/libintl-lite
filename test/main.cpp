#include <libintl.h>

int main()
{
    LoadMessageCatalog("messages", "data/cs/LC_MESSAGE/messages.mo");
    ngettext("I see {} dog", "I see {} dogs", 1);
    return 0;
}