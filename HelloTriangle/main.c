#include "app.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
        App app;
        Result result = appRun(&app);
        if (result.code != 0)
                fprintf(stderr, "Error: %s\n", (const char *) result.data);

        return result.code;
}
