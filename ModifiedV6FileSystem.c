/****************************************************************************************/
/*		UNIX V6 FILE SYSTEM											    				*/
/****************************************************************************************/
/*		Author	:KARPAGA GANESH PATCHIRAJAN												*/
/*				 SIDDHARTH PERUMAL														*/
/****************************************************************************************/
/*		Date	:04 DECEMBER, 2013	 													*/ 
/****************************************************************************************/

/******************************** HEADER FILES USED *************************************/
#include<stdio.h>
#include<sys/types.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>

/********************************* GLOBAL VARIABLES *************************************/
int test;
char tem[409600000];

/******************************* STRUCTURE DEFINITION ***********************************/
/** TESTER STRUCUTE **/
typedef struct {
unsigned int test;
} tester;

/** SUPER BLOCK STRUCTURE **/
typedef struct {
unsigned short isize;
unsigned short fsize;
unsigned short nfree;
unsigned short free[100];
unsigned short ninode;
unsigned short inode[100];
char flock;
char ilock;
char fmod;
unsigned short time[2];
} fs_super;

/** INODE STRUCTURE **/
typedef struct {
unsigned short flags;
char nlinks;
char uid;
char gid;
char size0;
unsigned int size1;
unsigned int addr[28];
unsigned short actime;
unsigned short modtime[2];
} fs_inode;

/** DIRECTORY STRUCTURE **/
typedef struct {
unsigned short inode;
char file_name[14];
} fs_directory;

/** FREE ARRAY STRUCTURE **/
typedef struct {
unsigned short nfree;
unsigned short free[100];
} fs_freeArray;

/** CREATING INSTANCES FOR STRUCTURES **/
fs_super super;
fs_inode inode;
fs_directory dir;
fs_freeArray free_array;
tester tell;

/****************************************************************************************/	
/*	METHOD USED TO FETCH A NEW UNUSED INODE 										    */
/****************************************************************************************/
/*	INPUT		: FILE DESCRIPTOR														*/
/*	OUTPUT		: INODE NUMBER															*/
/****************************************************************************************/	

int getiNode(int fd){

	int inode_num;
	lseek(fd, 2048, 0);
	read(fd,&super,2048);
	if(super.ninode == 0)
	{
		printf("\n\n$$ ERROR !!! OUT OF INODES !!! EXITING FILE SYSTEM !!!");
		exit(1);
	}	
	super.ninode--;
	inode_num = super.inode[super.ninode];
	lseek(fd, 2254, 0);
	write(fd, &super.ninode, 2);
	return inode_num;
}

/****************************************************************************************/	
/*	INITIALIZE THE NEW FILE SYSTEM. 		 										    */
/*	THIS CREATES SUPER BLOCK AND FILLS UP ROOT DIRECTORY INODE AND DATA					*/
/****************************************************************************************/
/*	INPUT		: FILE DESCRIPTOR,TOTAL NO.OF BLOCKS, TOTAL NO.OF INODES				*/
/*	OUTPUT		: 1(TRUE), NOT USED FOR ANY PURPOSE IN CODE								*/
/****************************************************************************************/		

int initialize_fs(int fd,unsigned short total_blcks,unsigned short total_inodes ){
	
	int isize, free, inode_no, i, available_data_blocks, upper_limit;	
	//Initialising the super block	
	if(((total_inodes*128)%2048) == 0)
		isize = ((total_inodes*128)/2048);
	else
		isize = ((total_inodes*128)/2048)+1;	
	super.isize=isize;
	super.fsize=total_blcks+1;
	super.nfree=100;
	free = isize + 3;
	available_data_blocks = (total_blcks - free);
	if(available_data_blocks < 100){
		upper_limit = available_data_blocks;
		available_data_blocks = 0;	
	}
	else{		
		upper_limit = 99;
		available_data_blocks = available_data_blocks-100;		
	}
	for(i=0; i<=upper_limit; i++)
		super.free[i]=free + i;
	super.ninode = 100;
	inode_no = 2;
	if(total_inodes < 101)
		upper_limit = total_inodes -1;
	else
		upper_limit = 100;
	super.ninode = upper_limit;

	for(i=0; i<upper_limit; i++)
		super.inode[i]=inode_no + i;
	super.flock = 'f';
	super.ilock = 'i';
	super.fmod = 'n';
	super.time[0] = 0000;
	super.time[1] = 1970;
	lseek(fd, 1*2048,0);
	write(fd, &super, 2048);
	
	/** INITIALIZING THE ROOT DIRECTORY INODE **/
	inode.flags = 0140777;
	inode.nlinks = '2';
	inode.uid = '0';
	inode.gid = '0';
	inode.size0 = '0';
	inode.size1 = 0;
	inode.addr[0] = free-1;
	inode.addr[1] = 0;
	inode.actime = 0;
	inode.modtime[0] = 0;
	inode.modtime[1] = 0;
	lseek(fd, 2*2048,0);
	write(fd, &inode, 128);
	
	dir.inode = 1;
	strncpy(dir.file_name, ".", 14);
	lseek(fd, (free-1)*2048, 0);
	write(fd, &dir, 16);
	strncpy(dir.file_name, "..", 14);
	write(fd, &dir, 16);
	dir.inode = 0;
	strncpy(dir.file_name, "0", 14);
	write(fd, &dir, 16);
		
	/** LINKING THE DATA BLOCKS **/
	while(available_data_blocks > 0){
		lseek(fd, (free*2048), 0);
		free += 100;
		if( available_data_blocks < 100){	
			upper_limit = available_data_blocks;
			free_array.nfree = available_data_blocks;
			available_data_blocks = 0;
		}
		else{
			upper_limit = 99;
			free_array.nfree = 100;
			available_data_blocks = available_data_blocks - 100;
		}
		for(i=0; i<=upper_limit; i++)
			free_array.free[i]=free + i;
		write(fd, &free_array, 202);
	}
	
	return 1;
}

/****************************************************************************************/	
/*	USED TO FETCH A NEW FREE DATA BLOCK													*/
/****************************************************************************************/
/*	INPUT		: FILE DESCRIPTOR														*/
/*	OUTPUT		: LOGICAL BLOCK NUMBER													*/
/****************************************************************************************/

int free_data_block(int fd){

	int logical_blocknum;
	lseek(fd, 2048, 0);
	read(fd, &super, 2048);
	
	/** FETCHING THE FREE DATA BLOCK FROM THE FREE[] WHEN NFREE IS NOT 1 **/
	if(super.nfree != 1){
		super.nfree--;
		logical_blocknum = super.free[super.nfree];	
		lseek(fd, 2052, 0);
		write(fd, &super.nfree, 2);
	}
	
	/** COPY NFREE AND FREE[] ARRAY FROM THE LINKED DATA BLOCK INTO THE SUPER BLOCK **/
	/** BEFORE ALLOCATING THAT DATA BLOCK WHEN NFREE BECOMES 1 **/ 

	else{
		super.nfree--;
		logical_blocknum = super.free[super.nfree];	
		lseek(fd, logical_blocknum *2048, 0);
		read(fd, &free_array, 202);
		lseek(fd, 2052, 0);
		write(fd, &free_array, 202);
	}
		
	return logical_blocknum;
}

/****************************************************************************************/	
/*	USED TO COPY AN EXTERNAL FILE TO A NEW MODIFIED V6 FILE SYSTEM						*/
/****************************************************************************************/
/*	INPUT		: FILE DESCRIPTOR, EXTERNAL FILE DESCRIPTOR, ARGUMENT 2					*/
/*	OUTPUT		: 1(TRUE), NOT USED FOR ANY PURPOSE IN CODE								*/
/****************************************************************************************/

int cpin(int fd,int extfd, char *arg2){

	int loop,seek,efSize,b,inodeNumber=0;
	int bytesRead=0,totalBytesRead=0,i=0;	
	int seekPosition,file_count;
	int garbageCollector = 0;
	int DataBlockNum = 0; 			
	int *buffer = (int *)malloc(2048);
	efSize = lseek(extfd, 0, SEEK_END);
	seek = lseek(extfd, 0, SEEK_SET);
	b = efSize/2048;
	garbageCollector = efSize%2048;
	if(seek < 0 || efSize < 0){
		printf("Error executing lseek in external file");
		return 0;;
	}
	if(b <= 28){
		loop = 1;	
	}
	else if(b/512 <= 28){
		loop = 2;
	} 
	lseek(fd, 2*2048,0);
	read(fd, &inode, 128);
	
	/** FINDING THE FILE COUNT INSIDE THE ROOT DIRECTORY **/
	file_count = 0;
	lseek(fd, (inode.addr[0]*2048), 0);
	read(fd, &dir, 16);
	while(dir.inode != 0){
		
		file_count++;
		read(fd, &dir, 16);
	}
	lseek(fd, (inode.addr[0]*2048)+(file_count*16), 0);
	dir.inode = inodeNumber;
	strncpy(dir.file_name, arg2, 14);
	write(fd, &dir, 16);
	int written_blocks;
	switch(loop){
		case 1:
				inodeNumber	 = getiNode(fd);
				seekPosition = (2*2048)+(128*(inodeNumber-1));
				inode.nlinks = '2';
				inode.uid = '0';
				inode.gid = '0';
				inode.size0 = '0';
				inode.size1 = efSize;
				inode.actime = 0;
				inode.modtime[0] = 0;
				inode.modtime[1] = 0;
				inode.flags = 0100777;
				lseek(fd,seekPosition,0);
				i=0;
				do{
					DataBlockNum = free_data_block(fd);
					inode.addr[i] = DataBlockNum;
					lseek(fd,seekPosition,0);
					written_blocks = write(fd,&inode,sizeof(inode));
					
					lseek(fd,seekPosition, 0);
					bytesRead = read(fd,&inode,sizeof(inode));
					if( b != 0)
						bytesRead = read(extfd,buffer,2048);
					else if(garbageCollector != 0)
						bytesRead = read(extfd,buffer,garbageCollector);
					else
					{	
						inode.addr[i] = 0;
						lseek(fd,seekPosition,0);
						written_blocks = write(fd,&inode,sizeof(inode));
						
					}
					b--;
					totalBytesRead += bytesRead;
					seekPosition = (DataBlockNum)*2048;
					lseek(fd,seekPosition,0);
					write(fd,buffer,bytesRead);	
					i++;
					seekPosition = (2*2048)+(128*(inodeNumber-1));
					lseek(fd,seekPosition,0);
					inode.addr[i]= DataBlockNum;		
				}while(bytesRead == 2048);
				
				inode.addr[i] = 0;
				lseek(fd,seekPosition,0);
				written_blocks = write(fd,&inode,sizeof(inode));			
				lseek(fd, 2*2048,0);
				read(fd, &inode, 128);
		
				/** FINDING THE FILE COUNT INSIDE THE ROOT DIRECTORY **/
				file_count = 0;
				lseek(fd, (inode.addr[0]*2048), 0);
				read(fd, &dir, 16);
				while(dir.inode != 0){
					file_count++;
					read(fd, &dir, 16);
				}
				lseek(fd, (inode.addr[0]*2048)+(file_count*16), 0);
				dir.inode = inodeNumber;
				strncpy(dir.file_name, arg2, 14);
				write(fd, &dir, 16);
		break;
		case 2:
				inodeNumber	 = getiNode(fd);
				seekPosition = (2*2048)+(128*(inodeNumber-1));
				inode.nlinks = '2';
				inode.uid = '0';
				inode.gid = '0';
				inode.size0 = '0';
				inode.size1 = efSize;
				inode.actime = 0;
				inode.modtime[0] = 0;
				inode.modtime[1] = 0;
				inode.flags = 0100777;
				int getIndirectDatablock,j,getDataBlock,written_blocks_total=0;
				seekPosition = (2*2048)+(128*(inodeNumber-1));
				i=0;
				do{			
					getIndirectDatablock = free_data_block(fd);
					inode.addr[i] = getIndirectDatablock;
					seekPosition = (2*2048)+(128*(inodeNumber-1));
					lseek(fd,seekPosition,0);
					written_blocks = write(fd,&inode,sizeof(inode));
					lseek(fd,seekPosition, 0);
					for(j=0;j<512 && totalBytesRead <29360128;j++){
						lseek(fd,getIndirectDatablock*2048+(j*4),0);
						getDataBlock = free_data_block(fd);
						tell.test = getDataBlock;
						seek = lseek(fd,getIndirectDatablock*2048+(j*4),0);
						write(fd,&tell,4);
						lseek(fd,getDataBlock*2048,0);
						bytesRead = read(extfd,buffer,2048);
						if (bytesRead == 0)
						break;
						totalBytesRead += bytesRead;
						lseek(fd,getDataBlock*2048,0);
						written_blocks=write(fd,buffer,2048);	
						written_blocks_total += written_blocks;
							//totalBytesRead = 99999;
					}
					i++;
				}while(bytesRead == 2048 && totalBytesRead <29360128 );
				seekPosition = (2*2048)+(128*(inodeNumber-1));
				lseek(fd,seekPosition,0);
				read(fd,&inode,128);
				seekPosition = (2*2048)+(128*(inodeNumber-1));						
				inode.addr[i] = 0;
				seekPosition = (2*2048)+(128*(inodeNumber-1));
				lseek(fd,seekPosition,0);
				written_blocks = write(fd,&inode,sizeof(inode));
				lseek(fd, 2*2048,0);
				read(fd, &inode, 128);
				
				/** FINDING THE FILE COUNT INSIDE THE ROOT DIRECTORY **/				
				file_count = 0;
				lseek(fd, (inode.addr[0]*2048), 0);
				read(fd, &dir, 16);
				while(dir.inode != 0){
					file_count++;
					read(fd, &dir, 16);
				}
				lseek(fd, (inode.addr[0]*2048)+(file_count*16), 0);
				dir.inode = inodeNumber;
				strncpy(dir.file_name, arg2, 14);
				write(fd, &dir, 16);
		break;
		default :
				printf("The file is too large.");
				break;
	}
	
	if(totalBytesRead >= 29360128){	
		seekPosition = (2*2048)+(128*(inodeNumber-1));
		lseek(fd,seekPosition,0);
		for(i=0;i<28;i++)
			inode.addr[i] = 0;
		lseek(fd,seekPosition,0);
		write(fd,&inode,sizeof(inode));	
	}
	seekPosition = (2*2048)+(128*(inodeNumber-1));		
	lseek(fd,seekPosition,0);
	read(fd,&inode,128);
	seekPosition = (2*2048)+(128*(inodeNumber-1));
	
	printf("\n## FILE SUCCESSFULLY COPIED TO V6 SYSTEM ##\n");
	return 1;
	
}

int decimal2octal(int n) {
    int rem, i=1, octal=0;
    while (n!=0){
        rem=n%8;
        n/=8;
        octal+=rem*i;
        i*=10;
    }
    
    return octal;
}

/****************************************************************************************/	
/*	USED TO COPY MODIFIED V6 FILE TO EXTERNAL DRIVE. 									*/
/****************************************************************************************/
/*	INPUT		: FILE DESCRIPTOR, ARG1-V6 SOURCE FILE, ARG2-PATH OF THE NEW EXT FILE	*/
/*	OUTPUT		: 1(TRUE), NOT USED FOR ANY PURPOSE IN CODE								*/
/****************************************************************************************/

int cpout(int fd, char *arg1, char *arg2){

	int file_count = 0;
	int flag = 0;
	int inodeNum, extfd, readBytes, controlVar, numberOfBlocks, blockNumber, writtenBytes, counter, logicalBlock;
	void *buffer = (void *)malloc(2048);	
	lseek(fd, 2*2048,0);
	read(fd, &inode, 128);		
	lseek(fd, (inode.addr[0]*2048), 0);
	read(fd, &dir, 16);
	while(dir.inode != 0){
		if(strcmp(arg1, dir.file_name)==0){	
			inodeNum = dir.inode;
			flag = 1;
			break;
		}			
		file_count++;
		read(fd, &dir, 16);
	}	
	if(flag != 1){
		printf("\n$$ ERROR ~ SOURCE FILE DOES NOT EXISTS $$\n");
		return 1;
	}
	else{
		printf("\n## CREATING NEW EXTERNAL FILE");
		extfd = open(arg2, O_RDWR | O_CREAT, 0755);
		lseek(fd, (2*2048)+((inodeNum-1)*128), 0);
		readBytes = read(fd, &inode, 128);
		if(readBytes <128)
			printf("\n\nerror reading source file inode");
		if(inode.size1 <= 57344){
			controlVar = 0;
			while(inode.addr[controlVar] != 0){
				numberOfBlocks++;
				controlVar++;
			}	
			controlVar = 0;
			while( controlVar < numberOfBlocks){	
				blockNumber = inode.addr[controlVar];
				/** READING FROM SOURCE FILE **/
				lseek(fd, blockNumber *2048, 0);
				readBytes = read(fd, buffer, 2048);
				/**WRITING TO DESTINATION FILE **/
				lseek(extfd, controlVar*2048, 0);
				writtenBytes = write(extfd, buffer, readBytes);
				controlVar++;
			}			
		}
		else if(inode.size1 <= 29360128){

			/** FINDING THE NUMBER OF BLOCKS USED **/
			controlVar = 0;
			while(inode.addr[controlVar] != 0){
				numberOfBlocks++;
				controlVar++;
			}	
			/** WRITING DATA TO EXTERNAL FILE **/
			controlVar = 0;
			while( controlVar < numberOfBlocks){
				blockNumber = inode.addr[controlVar];
				/** READING FROM SOURCE FILE **/ 
				counter = 0;
				void *buffer1 = (void *)malloc(4);
				do{
					lseek(fd, ((blockNumber*2048)+(counter*4)), 0);
					readBytes = read(fd, &tell, 4);
					logicalBlock = tell.test;
					counter++;
					// if(readBytes < 4)
// 						printf("error in reading logical block from indirect block");
					if(logicalBlock != 0){					
					/** READING FROM SOURCE FILE **/
						lseek(fd, logicalBlock *2048, 0);
						readBytes = read(fd, buffer, 2048);
						
						writtenBytes = write(extfd, buffer, 2048);
						
					}								
				}while((logicalBlock != 0) && (counter <= 512));								
				controlVar++;
			}			
		}
		else{
			printf("\nThe file is too large.");
		}		
	}
		
	printf("\n## FILE SUCCESSFULLY COPIED FROM V6 SYSTEM ##\n");	
	return 1;			
}

/****************************************************************************************/	
/*	MAIN FUNCTION									 									*/
/****************************************************************************************/
/*	INPUT		: COMMANDS (INITFS, CPIN, CPOUT, Q)										*/
/*	OUTPUT		: 1(TRUE), NOT USED FOR ANY PURPOSE IN CODE								*/
/****************************************************************************************/

int main(int args, char *arg[])
{
	int seekPosition,inodeNumber;
	int temp, i, blocks_read;
	int fd = -1;
	int extfd, flag, file_count, inode_pos;
	unsigned int number_of_blocks =0, number_of_inodes=0;
	unsigned short block_number, inode_number;
	char input[256];
	char *parser;
	char *path;
	char file_name[14];
	char *arg1, *arg2, *arg3;
	
	printf("\n## PLEASE ENTER ONE OF THE FOLLOWING COMMAND ##\n");
	printf("\n\t ~initfs - Initialization\n");
	printf("\t\t ~Example :: initfs /home/004/s/sx/sxg123456/V6 1000 3000 \n");
	printf("\n\t ~mkdir - Make Directory\n");
	printf("\t\t ~Example :: mkdir NewDirectory \n");
	printf("\n\t ~cpin - Copy a external file to File system\n");
	printf("\t\t ~Example :: cpin  /Users/karpagaganeshpatchirajan/Desktop/KG/Project_3/small.txt small\n");
	printf("\n\t ~cpout - Copy a file from File system to external path\n");
	printf("\t\t ~Example :: cpout  small /Users/karpagaganeshpatchirajan/Desktop/KG/Project_3/sm.txt \n");
	printf("\n\t ~q - Quit the File System\n");
	printf("\t\t ~Example :: q \n");
	printf("\n~");			
				
	/** LOOPING TO CONTINUOUSLY GET COMMAND FROM THE USER **/
	while( 1 )
	{	
		/** GETTING INPUT COMMAND FROM USER **/
		scanf(" %[^\n]s", input);
		
		/** CHECKING FOR QUIT COMMAND TO STOP GETTING INPUT FROM USER **/
		if(strcmp(input, "q") == 0)
			break;
					
		/** PARSING THE INPUT COMMAND TO FIND WHICH OPERATION TO PERFORM **/
		parser = strtok(input," ");
		if(strcmp(parser, "initfs")==0)
		{
			arg1 = strtok(NULL, " ");
			arg2 = strtok(NULL, " ");
			arg3 = strtok(NULL, " ");

			/** CHECKING FOR THE PRESENCE OF ALL THE ARGUMENTS **/
			if (!arg1 | !arg2 | !arg3)
			{
				printf("Please enter all initializing arguments(path for file system, total number of blocks and number of inodes).\n\n~");
				continue;
			}
			else{
				path = arg1;
				number_of_blocks = atoi(arg2);
				number_of_inodes = atoi(arg3);
				/** CHECK TO SEE IF THE FILE SYSTEM ALREADY EXISTS **/
				if( access(path, F_OK) == -1){
					fd = open(path, O_RDWR | O_CREAT, 0755);
					if(fd == -1)
					{
						printf("\n\n$$ ERROR INITIALIZING FILE SYSTEM. \n\nPlease check the entered path.");
						continue;
					}			
					printf("\nNew file system initialized.\n");
					initialize_fs(fd, number_of_blocks, number_of_inodes);
				}
				else{
					printf("\nFile system already exists. So loading the existing file system.\n");
					fd = open(path, O_RDWR, 0755);
				}
			}
		}
		else if(strcmp(parser, "cpin")==0){
			if(fd == -1){
				printf("\n\n$$ ERROR \nPlease initialize a file system first.\n\n~");
				continue;
			}
			arg1 = strtok(NULL, " ");
			arg2 = strtok(NULL, " ");
			if (!arg1 | !arg2 ){
				printf("\nPlease enter all the input parameters(source file and target file name).\n");
				continue;
			}
			else{
				extfd = open(arg1, O_RDWR, 0755);
				cpin( fd, extfd, arg2);
			}
		}
		else if(strcmp(parser, "cpout")==0){			
			if(fd == -1){
				printf("\n\n$$ ERROR \nPlease initialize a file system first.\n\n~");
				continue;
			}
			arg1 = strtok(NULL, " ");
			arg2 = strtok(NULL, " ");
			if (!arg1 | !arg2 ){
				printf("\nPlease enter all the input parameters(source file name and target path with file name).\n");
				continue;
			}
			else
				cpout(fd, arg1, arg2);			
		}
		else if(strcmp(parser, "test")==0)
		{
			if(fd == -1){
				printf("\n\n$$ ERROR \nPlease initialize a file system first.\n\n~");
				continue;
			}
			lseek(fd,2*2048,0);
			read(fd, &inode, 128);
			
			/** FINDING THE FILE COUNT INSIDE THE ROOT DIRECTORY **/
			file_count = 0;
			lseek(fd, (inode.addr[0]*2048), 0);
			read(fd, &dir, 16);
			while(dir.inode != 0){
				file_count++;
				read(fd, &dir, 16);
			}
			lseek(fd, (inode.addr[0]*2048)+(file_count*16), 0);
			dir.inode = 111;
			strncpy(dir.file_name, "test", 14);
			write(fd, &dir, 16);
			lseek(fd, (inode.addr[0]*2048)+((file_count+1)*16), 0);
			dir.inode = 0;
			strncpy(dir.file_name, "0", 14);
			write(fd, &dir, 16);
			lseek(fd, (2*2048)+(110*128), 0);
			inode.addr[0] = 50;
			inode.addr[1] = 0;
			inode.size1 = 75776;
			i = write(fd, &inode, 128);
			printf("bytes writ = %d", i);
			tell.test = 52;
			lseek(fd, 50*2048, 0);
			printf("\n *** %d ***", tell.test);
			write(fd, &tell, 4);
			tell.test = 53;
			write(fd, &tell, 4);
			tell.test = 54;
			write(fd, &tell, 4);
			for(i= 55; i < 90; i++){
				tell.test = i;
				write(fd, &tell, 4);
			}
			tell.test = 90;
			write(fd, &tell, 4);
			tell.test = 0000;
			write(fd, &tell, 4);
			printf("\n *** %d ***", tell.test);
			lseek(fd, 52*2048, 0);
			write(fd, "Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to dHello world sid here.", 2048);
			lseek(fd, 53*2048, 0);
			write(fd, "Hello world2 What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to dHello world sid here.", 2048);
			lseek(fd, 54*2048, 0);
			write(fd, "Hello world3 What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to dHello world sid here.", 2048);
			for(i= 55; i < 90; i++)
			{
			lseek(fd, i*2048, 0);
			write(fd, "Hello world4 What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to da is that repet.Hello world. What ader is rtying to da is that repet. Hello world. What ader is rtying to dHello world sid here.", 2048);
			}
			lseek(fd, 90*2048, 0);
			write(fd, "Mark Twain was born on the Missouri frontier and spent his childhood there. His real name is actually Samuel Langhorne Clemens. At the age of 12 he quit school in order to earn his living. At the age of 15 he already wrote his first article and by the time he was 16 he had his first short novel published. In 1857 he was an apprentice steamboat pilot on a boat that left Mississippi and was leading towards New Orleans. His characters were created because of the people and the situations he encountered on this trip.", 2048);
			
		}
		else if(strcmp(parser, "print")==0){
			if(fd == -1){
				printf("\n\n$$ ERROR \nPlease initialize a file system first.\n\n~");
				continue;
			}
			i = 0;
			lseek(fd, (2*2048)+(110*128), 0);
			read(fd, &inode, 128);
			i++;			
			lseek(fd, 50*2048, 0);
			void *buffer1 = (void *)malloc(2048);
			read(fd, buffer1,10);
		}	
		else if(strcmp(parser, "mkdir")==0){	
			if(fd == -1){
				printf("\n\n$$ ERROR \nPlease initialize a file system first.\n\n~");
				continue;
			}
			arg1 = strtok(NULL, " ");
			// Checking for the presence of all the arguments
			if (!arg1){
				printf("Please mention the directory name to be created.\n");
				continue;
			}
			else{
				lseek(fd, 2*2048,0);
				read(fd, &inode, 128);
				file_count = 0;
				flag =0;
				lseek(fd, (inode.addr[0]*2048), 0);
				read(fd, &dir, 16);
				while(dir.inode != 0){
					
					if(strcmp(arg1, dir.file_name)==0){
						flag = 1;
						break;
					}
					file_count++;
					read(fd, &dir, 16);
				}
				if(flag == 1)
					printf("$$ ERROR ~ DIRECTORY ALREADY EXISTS $$");
				else{
					block_number = free_data_block(fd);
					inode_number = getiNode(fd);
					
					/** ADDING ENTRY TO ROOT DIRECTORY **/
					lseek(fd, 4096, 0);
					read(fd, &inode, 128);
					lseek(fd, (inode.addr[0]*2048)+(file_count*16), 0);
					dir.inode = inode_number;
					strncpy(dir.file_name, arg1, 14);
					write(fd, &dir, 16);
					
					/** SETTING THE NEXT INODE ENTRY OF ROOT DIRECTORY LIST TABLE TO '0'**/
					dir.inode = 0;
					strncpy(dir.file_name, "0", 14);
					write(fd, &dir, 16);
					
					/** WRITING INTO THE INODE OF NEW DIRECTORY **/
					inode.flags = 0140755;
					inode.nlinks = '2';
					inode.uid = '0';
					inode.gid = '0';
					inode.size0 = '0';
					inode.size1 = 0;
					inode.addr[0] = block_number;
					inode.addr[1] = 0;
					inode.actime= 0;
					inode.modtime[0] = 0;
					inode.modtime[1] = 0;
					inode_pos = (2*2048) + ((inode_number-1) * 128);
					lseek(fd, inode_pos,0);
					write(fd, &inode, 128);
					
					/** WRITING THE DATA OF NEW DIRECTORY **/
					lseek(fd, block_number*2048, 0);
					dir.inode = inode_number;
					strncpy(dir.file_name, ".", 14);
					write(fd, &dir, 16);
					dir.inode = 1;
					strncpy(dir.file_name, "..", 14);
					write(fd, &dir, 16);
					
					//setting the next inode entry of the table to '0'.
					dir.inode = 0;
					strncpy(dir.file_name, "0", 14);
					write(fd, &dir, 16);					
					printf("\n## SUCCESSFULLY CREATED A NEW DIRECTORY ##\n");					
				}
			}
		}
		else{
			printf("\n\n$$ ERROR ~ INVALID COMMAND\n\n~PLEASE ENTER A VALID COMMAND\n~ ");
			continue;
		}
		/** DISPLAYS THE COMMAND PROMPT, FOR MORE RESPONSE FROM USER **/
		printf("\n/~");
	}
	
	printf("\n## EXITING THE FILE SYSTEM ##\n");
	
	return 1;

}
