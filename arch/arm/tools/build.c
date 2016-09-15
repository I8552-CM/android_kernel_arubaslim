#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
void load_menu(void);
void Compile(void);
void Flash(void);
void Clean(void);
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
        printf("1. Compile\n");
        printf("2. Flash\n");
	printf("3. Clean\n");
        printf("4. Exit\n");
        scanf("%d",&choice);
 
        switch(choice)
        {
            case 1: Compile();
                break;
            case 2: Flash();
                break;
	    case 3: Clean();
		break;
            case 4: printf("Quitting program!\n");
		unlink(build);
                exit(0);
                break;
            default: printf("Invalid choice!\n");
                break;
        }
 
    } while (choice != 3);
 
}
 
void Compile(void)
{
    		system("chmod 775 arch/arm/tools/scripts/compile; ./arch/arm/tools/scripts/compile");
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

