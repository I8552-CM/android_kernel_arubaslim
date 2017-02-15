#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

char *build = "build";
    
int main()
{
    		system("chmod 775 arch/arm/tools/scripts/compile; ./arch/arm/tools/scripts/compile");
		unlink(build);
                exit(0);
        return 0;
}

