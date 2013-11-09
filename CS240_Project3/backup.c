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

/*------------------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------*/

char writeDirToFile(FILE *fLN, char *cSD);
char *removeLastChar (char *str);
char *getType(unsigned char t);


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
	
	return 0;
}



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
	strcat(logFilePath, "/");					/* prepares the file path for a new file path */
	strcat(logFilePath, LOG_NEW_FILENAME);		/* adds new log filename to file path */
	FILE *logNew = fopen(logFilePath, "w+");	/* opens files for writing and reading */
//	DIR *srcDir = opendir(sourceDir);			/* creating directory based on the sourceDir name
//												 * passed into createLog() */
	writeDirToFile(logNew, sourceDir);
	
	fclose(logNew);		/* close logNew file */
//	closedir (srcDir);	/* close srcDir file */
}


char writeDirToFile(FILE *fLN, char *cSD){
	struct stat *st = malloc(sizeof(struct stat));
	struct dirent **nameList;
    int count = scandir(cSD, &nameList, NULL, alphasort);
	while(count--){
		stat(nameList[count]->d_name, st);	/* create statistics for directory */
		
		/* prints data type, data size, creation time, last modification time, and data name to log file */
		fprintf(fLN, "%s\t%ld\t%s\t%s\t%s\n",
				getType(nameList[count]->d_type),		/* DataType */
				(long)st->st_size,						/* DataSizeInBytes */
				removeLastChar(ctime(&(st->st_ctime))),	/* CreationTimeStamp */
				removeLastChar(ctime(&(st->st_mtime))),	/* ModificationTimeStamp */
				nameList[count]->d_name);				/* DataName */
		
		free(nameList[count]);
	}
	free(st);
	free(nameList);
	return 0;
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