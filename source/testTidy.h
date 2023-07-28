#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "libtidyhtml5/tidy.h"
#include "libtidyhtml5/tidybuffio.h"
#include <curl/curl.h>

uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out);
void dumpNode(TidyDoc doc, TidyNode tnod, int indent);
int tidymain(int argc, char **argv);

#ifdef __cplusplus
}
#endif
