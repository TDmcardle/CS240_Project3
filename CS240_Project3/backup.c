/**
 * Project 3 backup.c (C89)
 *
 * @author Tom McArdle
 *
 * @details The point of this project is to create a backup directory and a log file of directory
 *			backups.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>	/* directories library */
#include <dirent.h>		/* directories library  */
#include <sys/dir.h>
#include "backup.h"
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/*-----------------------------------FUNCTIONS----------------------------------------------------*/

char writeDirToFile(FILE *fLN, char *cSD);
char *removeLastChar (char *str);
char *getType(unsigned char t);
int *dontInclude(const struct dirent *d);
int sort_atoz(const void *i1, const void *i2);
void listdir(FILE *f, const char *name, int level);
char *removeFirstChar(char *cp, char c[], long i);
char *changeChar(char c[], char remove, char add);

/*----------------------------------GLOBAL VARIABLES----------------------------------------------*/

int logIsSame = 0;
char workingDir[200];
char *cpWD = &workingDir[0];

/*------------------------------------------------------------------------------------------------*/
/*-----------------------------------MAIN---------------------------------------------------------*/


int main(int argc, char * argv[]){
	DIR *dSD = NULL;					/* source directory */
	DIR *dDD = NULL;					/* destination directory */
	long maxBackups = 0;
	char srcDirName[FILENAME_MAX];		/* source directory name */
	char *pSDN = &srcDirName[0];		/* pointer to the source directory name */
	char destDirName[FILENAME_MAX];		/* destination directory name */
	char *pDDN = &destDirName[0];		/* pointer to the destination directory name */
	int i = 0;
	

	/*---------------------------------ARGS-------------------------------------------------------*/
	/**
	 * Search the arguments for source directory (-s), destination directory (-d), and maximum
	 * number of backup instances (-m)
	 */
	for (i=0; i<argc; i++){
#ifdef debug_args
		printf("arg (%d): %s\n", i, argv[i]);
#endif
		/* (required) Search for sorce directory */
		if (strcmp(argv[i], "-s") == 0){
			pSDN = argv[i+1];
		}
		
		/* (optional) Search for destination directory */
		if (strcmp(argv[i], "-d") == 0){
			pDDN = argv[i+1];
		}
		
		/* (optional) Maximum number of backup instances to save */
		if (strcmp(argv[i], "-m") == 0){
			maxBackups = atol(argv[i+1]);
		}
	}

	if (maxBackups == 0)					/* If the maximum number of backups were not defined, */
		maxBackups = DEFAULT_MAX_BACKUPS;	/* then the default value is used. */

	if (pDDN == NULL){				/* If destination dierectory name not defined */
		pDDN = DEFAULT_DEST_DIR;	/* then define the destination name to the default name */
		dDD = opendir(pDDN);		/* and open the default directory */
	}
	else{
		dDD = opendir(pDDN);		/* else open the directory defined in the arguments */
	}
	
#ifdef debug_args
	if (pSDN != NULL) {
		while (ep = readdir (dSD))
			printf("sourceDir: %s\n", ep->d_name);
		closedir (dSD);
	}
	else
		perror ("Couldn't open the current directory");
#endif

	if ((dSD = opendir(pSDN))) {
		fprintf(stdout, "Usage: \n./backup -s %s [-d %s -m %ld]\n", pSDN, pDDN, maxBackups);
		closedir(dSD);
	}
	else{
		fprintf(stderr, "missing: -s or sourceDir\n");
		return 1;
	}
	/*---------------------------------END ARGS-----------------------------------------------*/


	
	createLog(pSDN, pDDN);
	
	copyDir(pSDN, pDDN);
	
	closedir(dDD);
	return 0;
}


/*------------------------------------------------------------------------------------------------*/
/*-----------------------------------CREATE LOG---------------------------------------------------*/


/**
 * @function	void createLog(char *sourceDir, char *logFilePath)
 * @details	Creates a log file of source directory and stores the log file into the destination
 *			directory. Log file created in the following format (each entry on signle line):
 *				<DataType><tab><DataSizeInBytes><tab><CreationTimeStamp><tab>
 *				<LastModificationTimeStamp><tab><DataName><newline>
 *			Directory laying uses the following format:
 *				<ParentDirectoryInfo><newline>
 *				<tab><SubDirectoryInfo><newline>
 *			Files and directories of the same path are aligned and each sub-directory is indented.
 * @param	char *sourceDir		source directory name
 * @param	char *logFilePath	destination directory name used to store the logfile
 */
void createLog(char *sourceDir, char *logFilePath){
	char pathNew[100];
	snprintf(pathNew, sizeof(pathNew)-1, "%s/%s", logFilePath, LOG_NEW_FILENAME);
	FILE *logNew = fopen(pathNew, "w+");	/* opens files for writing and reading */
	char pathOld[100];
	snprintf(pathOld, sizeof(pathOld)-1, "%s/%s", logFilePath, LOG_LAST_FILENAME);
	FILE *logOld = fopen(pathOld, "w+");	/* opens files for writing and reading */
	
	listdir(logNew, sourceDir, 0);

	logIsSame = compareLog(logOld, logNew);
#ifdef debug_compareLog
	fprintf(stderr, "Log comparison is(1=same,0=different): %d\n", logIsSame);
#endif
	
	/* compute the size of the file */
//	fseek(logNew, 0, SEEK_END) ;
//	fsizeNew = ftell(logNew);

	if (!logIsSame){
		long lSize;
		char * buffer;
				
		/* obtain file size: */
		fseek (logNew , 0 , SEEK_END);
		lSize = ftell (logNew);
		rewind (logNew);
		
		/* allocate memory to contain the whole file: */
		buffer = (char*) malloc (sizeof(char)*lSize);
		
		/* copy the file into the buffer: */
		fread (buffer, 1, lSize, logNew);
		fwrite(buffer, 1, lSize, logOld);

		free (buffer);
	}
	
	
	fclose(logNew);		/* close logNew file */
	fclose(logOld);		/* close logOld file */
}


/*------------------------------------COMPARE LOGS------------------------------------------------*/

/**
 * @return	1	Files are identical
 * @return	0	Otherwise
 */
int compareLog(FILE *oldLogFile, FILE *newLogFile){
	long fsizeNew, fsizeOld;
	long lSize = 0;
	char * bufferNew;
	char * bufferOld;
	
	/* allocate memory to contain the whole file: */
	bufferNew = (char*) malloc (sizeof(char)*lSize);
	bufferOld = (char*) malloc (sizeof(char)*lSize);
	
	/* compute the size of the file */
	fseek(newLogFile, 0, SEEK_END) ;
	fsizeNew = ftell(newLogFile);
	rewind(newLogFile);
	
	fseek(oldLogFile, 0, SEEK_END) ;
	fsizeOld = ftell(oldLogFile);
	rewind(oldLogFile);
	
#ifdef debug_compareLog
	fprintf(stderr, "New log size is: %ld\n", fsizeNew);
	fprintf(stderr, "Old log size is: %ld\n", fsizeOld);
#endif
	
	if (fsizeOld-fsizeNew)
		return 0;
	
	fread (bufferNew,1,lSize,newLogFile);
	fread (bufferOld,1,lSize,oldLogFile);
	
	if (strcmp(bufferNew, bufferOld))
		return 0;
	else
		return 1;
}


/*------------------------------------COPY FILE---------------------------------------------------*/

/**
 * @function	int copyFile(char *sourcePath, char *destinationPath)
 * @details		Copy file from source path to destination path
 * @param	char *sourcePath		Copy files from this directory path
 * @param	char *destinationPath	Copy the source files to this directory path
 * @return	1						If successful copy
 * @return	0						Otherwise
 */
int copyFile(char *sourcePath, char *destinationPath){
//	char pathSrc[100];
//	snprintf(path, sizeof(pathOld)-1, "%s/%s", logFilePath, LOG_LAST_FILENAME);
//	FILE *logOld = fopen(pathSrc, "w+");	/* opens files for writing and reading */
//
//	
//	
//	long lSize;
//	char * buffer;
//	
//	/* obtain file size: */
//	fseek (logNew , 0 , SEEK_END);
//	lSize = ftell (logNew);
//	rewind (logNew);
//	
//	/* allocate memory to contain the whole file: */
//	buffer = (char*) malloc (sizeof(char)*lSize);
//	
//	/* copy the file into the buffer: */
//	fread (buffer, 1, lSize, logNew);
//	fwrite(buffer, 1, lSize, logOld);
//	
//	free (buffer);
//	fclose(logNew);		/* close logNew file */
//	fclose(logOld);		/* close logOld file */

	return 0;
}


/*-----------------------------------COPY DIR-----------------------------------------------------*/

/**
 * @function	int copyDir(char *sourceDir, char *backupDir)
 * @details		Copy format of source directory to backup directory and use copyFile to copy the
 *				files.
 * @param	char *sourceDir		Source directory
 * @param	char *backupDir		destination directory
 * @return	1	If copy is successful
 * @return	0	Otherwise
 */
int copyDir(char *sourceDir, char *backupDir){
	
    struct dirent **entry;
	struct stat *st = (struct stat*) malloc(sizeof(struct stat));
	
	char *t_YMD = (char *) malloc(sizeof(13));
	char *t_HMS = (char *) malloc(sizeof(9));
	
	/* Create backup directory if it does not exist */
	char prePath[100];
	int n = scandir(sourceDir, &entry,NULL, alphasort);
	strftime(t_YMD, 13, "%F", localtime(&(st->st_ctime)));
	strftime(t_HMS, 9, "%T", localtime(&(st->st_ctime)));
	t_HMS = changeChar(t_HMS, ':', '-');
	snprintf(prePath, sizeof(prePath)-1, "%s/%s-%s", backupDir, t_YMD, t_HMS);
	printf("\nBackup Directory: %s\nprePath: %s\n\n", backupDir, prePath);
	/* The behavior of mkdir is undefined for anything other than the "permission" bits */
	if (mkdir(prePath, 0777))
		perror(prePath);
	/* So we need to set the sticky/executable bits explicitly with chmod after calling mkdir */
	if (chmod(prePath, 07777))
		perror(prePath);
	

	while(n--){
        if (entry[n]->d_type == DT_DIR) {
            char path[150];
			stat(entry[n]->d_name, st);	/* create statistics for directory */
            int len = snprintf(path, sizeof(path)-1, "%s/%s", prePath, entry[n]->d_name);
            if (strcmp(entry[n]->d_name, ".") == 0 || strcmp(entry[n]->d_name, "..") == 0)
                continue;
			/* The behavior of mkdir is undefined for anything other than the "permission" bits */
			if (mkdir(path, 0777))
				perror(path);
			
			/* So we need to set the sticky/executable bits explicitly with chmod after calling mkdir */
			if (chmod(path, 07777))
				perror(path);
//            copyDir(entry[n]->d_name, path);
		}
//		else
//		copyFile();
		
	}
	free(st);
	free(t_YMD);
	free(t_HMS);
	return 0;
}


/*-----------------------------------HELPER FUNCTIONS---------------------------------------------*/

char *changeChar(char c[], char remove, char add){
	int i = 0;
	for (i=0; i<strlen(c); i++){
		if (c[i] == remove){
			c[i] = add;
		}
	}
	return c;
}


void listdir(FILE *f, const char *name, int level){
    DIR *dir;
    struct dirent **entry;
	struct stat *st = (struct stat*) malloc(sizeof(struct stat));
    if (!(dir = opendir(name)))
        return;
	int n = scandir(name, &entry,NULL, alphasort);
	
	while(n--){
		stat(entry[n]->d_name, st);	/* create statistics for directory */
        if (entry[n]->d_type == DT_DIR) {
            char path[100];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry[n]->d_name);
            path[len] = 0;
            if (strcmp(entry[n]->d_name, ".") == 0 || strcmp(entry[n]->d_name, "..") == 0)
                continue;
			fprintf(f, "%*s%s\t%ld\t%s\t%s\t%s\n",
					level*4, "",
					getType(entry[n]->d_type),
					(long)st->st_size,
					removeLastChar(ctime(&(st->st_ctime))),
					removeLastChar(ctime(&(st->st_mtime))),
					entry[n]->d_name);
            listdir(f, path, level + 1);
        }
		else
			fprintf(f, "%*s%s\t%ld\t%s\t%s\t%s\n",
					level*4, "",
					getType(entry[n]->d_type),
					(long)st->st_size,
					removeLastChar(ctime(&(st->st_ctime))),
					removeLastChar(ctime(&(st->st_mtime))),
					entry[n]->d_name);
	}
    closedir(dir);
	free(st);
}

int sort_atoz(const void *i1, const void *i2){
	
	int item1 = *(int*)i1;
	int item2 = *(int*)i2;
	
	if(item1 < item2){
		return 1;
	}
	else if(item1 == item2){
		return 0;
	}
	else{
		return -1;
	}
}

/**
 * @function	char *removeLastChar (char str[])
 * @details		Replaces the last character of a string with '\0', indicating end of string
 * @param	char str[]	Character array
 * @return	char *str	Character pointer of the string passed in with the last char removed
 */
char *removeLastChar (char str[]){
	str[strlen(str)-1] = '\0';
	return str;
}


/**
 * @function	char *removeFirstChar(char *cp, char c[], long i)
 * @details		recursively shifts characters to the left to delete first character and adds '\0'
 *				to the end of the string
 * @param	char *cp	Character pointer, string to remove first character of
 * @param	char c[]	Used to duplicate the string with the removed character
 * @param	long i		Tracks place in recursion
 * @return	char *c		pointer to the string with the first character removed
 */
char *removeFirstChar(char *cp, char c[], long i){
	if (i < strlen(cp)){
		c[i] = *(cp+i+1);
		return removeFirstChar(cp, c, ++i);
	}
	else{
		c[i] = '\0';
		return c;
	}
}

/**
 * @function	char *getType(unsigned char t)
 * @details		Converts the data type hexidecimal value to a string of the #define type
 * @param	unsigned char t		Hexidecimal value passed in to be converted to string
 * @return	char *				String of data type
 */
char *getType(unsigned char t){
	switch (t){
		case DT_UNKNOWN: return "DT_UNKNOWN";
		case DT_FIFO: return "DT_FIFO";
		case DT_CHR: return "DT_CHR";
		case DT_DIR: return "DT_DIR";
		case DT_BLK: return "DT_BLK";
		case DT_REG: return "DT_REG";
		case DT_LNK: return "DT_LNK";
		case DT_SOCK: return "DT_SOCK";
		case DT_WHT: return "DT_WHT";
		default: return "";
	}
}


















/* Space used to keep code of bottom of screen. It bugs me. */