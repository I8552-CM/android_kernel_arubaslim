#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
void load_menu(void);
void delos(void);
void Flash(void);
void Clean(void);
void aurba(void);
char *build = "build";
int main(int argc, char** argv)
{
    load_menu();
    return 0;
}
 
void load_menu(void)
{
    int choice;
 
    do
    {
   system("echo 'MSM7627a'");
        printf("1. Compile for delos3geur\n");
	printf("2. compile for arubaslim\n");
        printf("3. Flash\n");
	printf("4. Clean\n");
        printf("5. Exit\n");
        scanf("%d",&choice);
 
        switch(choice)
        {
            case 1: delos();
                break;
            case 2: aurba();
                break;
            case 3: Flash();
                break;
	    case 4: Clean();
		break;
            case 5: printf("Quitting program!\n");
		unlink(build);
                exit(0);
                break;
            default: printf("Invalid choice!\n");
                break;
        }
 
    } while (choice != 4);
 
}
 
void delos(void)
{
    		system("chmod 775 arch/arm/tools/scripts/compile-delos; ./arch/arm/tools/scripts/compile-delos");
		unlink(build);
                exit(0);

}     

void aurba(void)
{
    		system("chmod 775 arch/arm/tools/scripts/compile-aurba; ./arch/arm/tools/scripts/compile-aurba");
		unlink(build);
                exit(0);

}     


void Flash(void)
{

    		system("chmod 775 arch/arm/tools/scripts/flash; ./arch/arm/tools/scripts/flash");
		unlink(build);
                exit(0);
return;
}
void Clean(void)
{
    		system("make mrproper");
		unlink(build);
                exit(0);
return;
}

