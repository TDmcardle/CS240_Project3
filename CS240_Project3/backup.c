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
void copyDirHelper(char *cpSD, char *cpPP);
void removeFilesInDir(char *destinationDir);
char *addTabs(int i, char t[]);


/*----------------------------------GLOBAL VARIABLES----------------------------------------------*/

int logIsSame = 0;
long maxBackups = 0;


/*------------------------------------------------------------------------------------------------*/
/*-----------------------------------MAIN---------------------------------------------------------*/


int main(int argc, char * argv[]){
	DIR *dSD = NULL;					/* source directory */
	DIR *dDD = NULL;					/* destination directory */
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
	
	if (!logIsSame)
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
	char pathNew[200] = "";
	snprintf(pathNew, sizeof(pathNew), "%s/%s", logFilePath, LOG_NEW_FILENAME);
	FILE *logNew = NULL;
	logNew = fopen(pathNew, "w+");	/* opens files for writing and reading */
	
	char pathOld[200] = "";
	snprintf(pathOld, sizeof(pathOld), "%s/%s", logFilePath, LOG_LAST_FILENAME);
	FILE *logOld = NULL;
	logOld = fopen(pathOld, "r");	/* opens files for writing and reading */
	
	listdir(logNew, sourceDir, 0);			/* Creates the new log file */

	/* If the old and new logs are different, then update the old log */
	if (!(logIsSame = compareLog(logOld, logNew))){
		long lSize;
		char * buffer;
		
		fclose(logOld);
		logOld = fopen(pathOld, "w+");
				
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
	
#ifdef debug_compareLog
	fprintf(stderr, "Log comparison is(1=same,0=different): %d\n", logIsSame);
#endif
	
	fclose(logNew);		/* close logNew file */
	fclose(logOld);		/* close logOld file */
}


/*------------------------------------COMPARE LOGS------------------------------------------------*/

/**
 * @return	1	Files are identical
 * @return	0	Otherwise
 */
int compareLog(FILE *oldLogFile, FILE *newLogFile){
	long fsizeNew = 0;
	long fsizeOld = 0;
	long lSize = 0;
	int ret = 0;

	
	/* allocate memory to contain the whole file: */
	char * bufferNew = (char*) malloc (sizeof(char)*lSize);
	char * bufferOld = (char*) malloc (sizeof(char)*lSize);
	
	/* compute the size of the new log file */
	fseek(newLogFile, 0, SEEK_END) ;
	fsizeNew = ftell(newLogFile);
	rewind(newLogFile);
	
	/* compute the size of the old log file */
	fseek(oldLogFile, 0, SEEK_END) ;
	fsizeOld = ftell(oldLogFile);
	rewind(oldLogFile);
	
#ifdef debug_compareLog
	fprintf(stderr, "New log size is: %ld\n", fsizeNew);
	fprintf(stderr, "Old log size is: %ld\n", fsizeOld);
#endif
	
	fread (bufferNew,1,lSize,newLogFile);
	fread (bufferOld,1,lSize,oldLogFile);
	
	if ((fsizeOld-fsizeNew) || strcmp(bufferNew, bufferOld))
		ret = 0;
	else
		ret = 1;
	
	free(bufferNew);
	free(bufferOld);
	
	return ret;
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
	FILE *srcFile = fopen(sourcePath, "r");			/* opens source file for reading */
	FILE *destFile = fopen(destinationPath, "w");	/* opens destination file for writing */
	
	long lSize = 0;
	
	/* obtain file size: */
	fseek (srcFile , 0 , SEEK_END);
	lSize = ftell (srcFile);
	rewind (srcFile);
	
	char * buffer = (char*) malloc (sizeof(char)*lSize);	/* allocate memory to contain the whole file: */
	fread (buffer, 1, lSize, srcFile);				/* copy the source file into the buffer */
	fwrite(buffer, 1, lSize, destFile);				/* write buffer to destination file */
	
	free (buffer);			/* free bufer */
	fclose(srcFile);		/* close logNew file */
	fclose(destFile);		/* close logOld file */

	return 1;
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
	
	struct stat *st = (struct stat*) malloc(sizeof(struct stat));
	stat(sourceDir, st);	/* create statistics for directory */


	char *t_YMD = (char *) malloc(sizeof(13));
	char *t_HMS = (char *) malloc(sizeof(9));
	
	/* Create backup directory if it does not exist */
	char prePath[200] = "";
	strftime(t_YMD, 13, "%F", localtime(&(st->st_mtime)));
	strftime(t_HMS, 9, "%T", localtime(&(st->st_mtime)));
	t_HMS = changeChar(t_HMS, ':', '-');
	snprintf(prePath, sizeof(prePath)-1, "%s/%s-%s", backupDir, t_YMD, t_HMS);
#ifdef debug_copyDir
	printf("\nSource Directory: %s\nBackup Directory: %s\nprePath: %s\n\n", sourceDir, backupDir, prePath);
#endif
	
//	if (((getNumOfBackup(backupDir)+1)>maxBackups)){	/* If more backups (including new backup) than max */
//		removeOldestBackup(backupDir);					/* then remove oldest backup */
//	}
	
#ifdef debug_copyDir
	int err = 0;
	fprintf(stderr, "\n*** Debugging copyDir (in copyDir) ***\n");
	/* The behavior of mkdir is undefined for anything other than the "permission" bits */
	if ((err = mkdir(prePath, 0777)))
		perror(prePath);
	fprintf(stderr, "mkdir: %d\n", err);
	/* So we need to set the sticky/executable bits explicitly with chmod after calling mkdir */
	if ((err = chmod(prePath, 07777)))
		perror(prePath);
	fprintf(stderr, "chmod: %d\n", err);
	fprintf(stderr, "*** END Debugging copyDir (in copyDir) ***\n\n");
#else
	mkdir(prePath, 0777);
	chmod(prePath, 07777);
#endif
	
	free(st);

	
	copyDirHelper(sourceDir, prePath);
	
	
	free(t_YMD);
	free(t_HMS);
	return 1;
}


/*------------------------------GET NUMBER OF BACKUPS---------------------------------------------*/
/**
 * @return	int i	Number of existing backups under destinationDir
 */
int getNumOfBackup(char *destinationDir){
	int i = 0;
	struct dirent **entry;
	int n = scandir(destinationDir, &entry, NULL, NULL);
	while(n--){
		if (entry[n]->d_type == DT_DIR) {
			i++;
		}
	}
	return i;
}


/*------------------------------REMOVE OLDEST BACKUP----------------------------------------------*/

/**
 * @details	recursively deletes backups if number of backups in file is larger than the difined maxBackups.
 */
int removeOldestBackup(char *destinationDir){
	DIR *dir = NULL;
	if ((dir = opendir(destinationDir)))
		return 0;
	closedir(dir);
	struct dirent **entry;
	int n = scandir(destinationDir, &entry, NULL, alphasort);	/* sorts files so oldest is first */

	while(n--){
		/* creates string with destination directory plus the next file/directory name */
		char backupPath[200];
		snprintf(backupPath, sizeof(backupPath)-1, "%s/%s", destinationDir, entry[n]->d_name);

		if (entry[n]->d_type == DT_DIR) {
			if ((getNumOfBackup(destinationDir)>maxBackups)){	/* If more backups than max */
				removeFilesInDir(backupPath);
				remove(entry[n]->d_name);			/* removes backups */
//				removeOldestBackup(destinationDir);	/* recursively deletes backups */
			}
		}
	}
	
	return 0;
}


/*-----------------------------------HELPER FUNCTIONS---------------------------------------------*/

/**
 * @details		Removes all files within a directory.
 */
void removeFilesInDir(char *destinationDir){
	struct dirent **entry = NULL;
	int n = scandir(destinationDir, &entry, NULL, NULL);

	while(n--){
		/* creates string with destination directory plus the next file/directory name */
		char backupPath[200];
		snprintf(backupPath, sizeof(backupPath)-1, "%s/%s", destinationDir, entry[n]->d_name);
		
		if (entry[n]->d_type != DT_DIR) {
			remove(entry[n]->d_name);			/* removes backups */
			
		}
	}
}

/**
 * @details		Recursively copies each file and directory from source directory to the destination
 *				directory.
 */
void copyDirHelper(char *srcDir, char *destDir){
	DIR *dir = NULL;
	DIR *dir2 = NULL;
	if (!(dir = opendir(srcDir)))
        return;
	closedir(dir);
	
	if (!(dir2 = opendir(destDir)))
        return;
	closedir(dir2);
	
	struct dirent **entry = NULL;
	int n = scandir(srcDir, &entry, NULL, NULL);
	struct stat *st = (struct stat*) malloc(sizeof(struct stat));

	while(n--){
		stat(entry[n]->d_name, st);	/* create statistics for directory */
		
		char srcPath[200] = "";
		char backupPath[200] = "";
		snprintf(srcPath, sizeof(srcPath)-1, "%s/%s", srcDir, entry[n]->d_name);
		snprintf(backupPath, sizeof(backupPath)-1, "%s/%s", destDir, entry[n]->d_name);
        
		if (entry[n]->d_type == DT_DIR) {
            if (strcmp(entry[n]->d_name, ".") == 0 || strcmp(entry[n]->d_name, "..") == 0)
                continue;
#ifdef debug_copyDir
			int err = 0;
			fprintf(stderr, "\n*** Debugging copyDir (in copyDirHelper) ***\n");
			/* The behavior of mkdir is undefined for anything other than the "permission" bits */
			if ((err = mkdir(backupPath, 0777)))
				perror(backupPath);
			fprintf(stderr, "mkdir: %d\n", err);
			/* So we need to set the sticky/executable bits explicitly with chmod after calling mkdir */
			if ((err = chmod(backupPath, 07777)))
				perror(backupPath);
			fprintf(stderr, "chmod: %d\n", err);
			fprintf(stderr, "*** END Debugging copyDir (in copyDirHelper) ***\n\n");
#else
			mkdir(backupPath, 0777);
			chmod(backupPath, 07777);
#endif
			copyDirHelper(srcPath, backupPath);
		}
		else{
			copyFile(srcPath, backupPath);
		}
	}
	
	free(st);
}

/**
 * @details		Changes a character in the specified array/string
 */
char *changeChar(char c[], char remove, char add){
	int i = 0;
	for (i=0; i<strlen(c); i++){
		if (c[i] == remove){
			c[i] = add;
		}
	}
	return c;
}

/**
 * @details		Used to recursively create the directories and files
 */
void listdir(FILE *f, const char *name, int level){
    DIR *dir = NULL;
    struct dirent **entry = NULL;
	struct stat *st = (struct stat*) malloc(sizeof(struct stat));
    if (!(dir = opendir(name)))
        return;
	int n = scandir(name, &entry,NULL, alphasort);
	char t_std[50] = ""; /* represents the national time or standard time format */
	while(n--){
		char path[200] = "";
		snprintf(path, sizeof(path)-1, "%s/%s", name, entry[n]->d_name);
		
		stat(path, st);	/* create statistics for directory */
		strftime(t_std, sizeof(t_std), "%c", localtime(&(st->st_mtime)));
		char tabs[50] = "";
        if (entry[n]->d_type == DT_DIR) {
            if (strcmp(entry[n]->d_name, ".") == 0 || strcmp(entry[n]->d_name, "..") == 0)
                continue;
			fprintf(f, "%s%s\t%ld\t%s\t%s\t%s\n",
					addTabs(level, tabs),
					getType(entry[n]->d_type),
					(long)st->st_size,
					t_std,
					t_std,
					entry[n]->d_name);
            listdir(f, path, level + 1);
        }
		else{
			fprintf(f, "%s%s\t%ld\t%s\t%s\t%s\n",
					addTabs(level, tabs),
					getType(entry[n]->d_type),
					(long)st->st_size,
					t_std,
					t_std,
					entry[n]->d_name);
		}

	}
    closedir(dir);
	free(st);
}


/**
 * @details		Sorts the two parameters in ascending alphabetical order (a-z) (opposite of alphasort)
 */
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

/**
 * @function	char *addTabs(int i, char *t)
 * @details		Adds i number of tabs
 * @param	int i		number of tabs
 * @param	char *t		string to add tabs to
 */
char *addTabs(int i, char *t){
	int ntabs = i;
	while (ntabs--){
		snprintf(t, sizeof(t), "%s\t", t);
	}
	return t;
}
















/* Space used to keep code of bottom of screen. It bugs me. */