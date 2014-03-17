#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h> 

void ListFiles(char *dirName)
{
  DIR           *d;
  struct dirent *dir;
  struct stat inode;
  char name[500];
  
  d = opendir(dirName); 

   if (d == 0) 
  {
      perror (""); //inbuilt function for system defined errors
  }
  else if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
       sprintf(name,"%s/%s",dirName,dir->d_name); //prints in system buffer

	lstat (name, &inode) ; //'lstat' is a system call. In this name=full file path, lstat returns 0 on success and -1 on failure and throws respective error, inode is a structure. 

       if (S_ISDIR(inode.st_mode)) //S_ISDIR is Macro 
          printf("[d");
       else if (S_ISREG(inode.st_mode)) //Macro
          printf ("[-");
       else if (S_ISLNK(inode.st_mode)) //Macro
          printf ("[l");
       else;
	
    printf( (inode.st_mode & S_IRUSR) ? "r" : "-"); //Checking and displaying the read,write and execute permissions of USER
    printf( (inode.st_mode & S_IWUSR) ? "w" : "-");
    printf( (inode.st_mode & S_IXUSR) ? "x" : "-");

    printf( (inode.st_mode & S_IRGRP) ? "r" : "-"); //Checking and displaying the read,write and execute permissions of GROUP
    printf( (inode.st_mode & S_IWGRP) ? "w" : "-");
    printf( (inode.st_mode & S_IXGRP) ? "x" : "-");

    printf( (inode.st_mode & S_IROTH) ? "r" : "-"); //Checking and displaying the read,write and execute permissions of OTHERS
    printf( (inode.st_mode & S_IWOTH) ? "w" : "-");
    printf( (inode.st_mode & S_IXOTH) ? "x" : "-");
    
    printf("]  %s\n", dir->d_name);  //prints on console till while loop executes.
    }

    closedir(d);
  }
}

int main(int argc, char **argv)
{
    if (argc == 1) 
      {ListFiles(".");}
    else
      {ListFiles(argv[1]);}

  return(0);
}