/*****************************************************************************
* NAME : SimpleCOM.cpp
* DESC : Simple serial port test program - single loop
* DATE : 7/15/2003
* PGMR : Y. Bai
*****************************************************************************/
#include "SimpleCOM.h"
#include "Menu.h"
int main()
{
	int index;
	//Boolean variable used to understed whether the loop continues to run or stop
	bool select = TRUE;
	//receive user selection
	char userInput[16];
	__int8 movement;
	ERR_CODE ecRet = OK;
	unsigned char movement_code;
	char const comPortName[] = "\\\\.\\COM10";
	char const fileName[] = "BinaryDataset.bin";
	char const mode[] = "wb";
	TestPassed = FALSE;


	//File to which write the received characters
    binaryFile = fopen( fileName, mode );
	//Continue to test as long as the process is free of errors.

	//Put a name for the dataset
	char name[NAMELENGTH];
	//Write all zeros, then scanf will overwrite.
	for(int N = 0;N < NAMELENGTH; N++) name[N] = 0;

	scanf("%s",name);

    int offset;
    //Some macro helps to set the correct offset

    //Write the name on the file
    NAMEOFFSET(offset)
        fseek(binaryFile,offset,SEEK_SET);
        for(int pos_in_name = 0;pos_in_name < NAMELENGTH; pos_in_name++) fwrite((void*)&name[pos_in_name],sizeof(char),1,binaryFile);

    //Write all zeros to the size section, since this will be updated after quitting the while loop
    SIZEOFFSET(offset)
        fseek(binaryFile,offset,SEEK_SET);
        char filler = 0;
        for(int pos_in_size = 0; pos_in_size < SIZELEGHT; pos_in_size++) fwrite((void*)&filler,sizeof(char),1,binaryFile);

    //Do the actual initialization of the port.
    ecRet = SetupPort(comPortName);
    if (ecRet)
    {
        //Break the while if no successfull initialization
        select = FALSE;
    }

    unsigned int counter_of_movements = 0;

    //Positioning on the file where the data must start
    DATAOFFSET(offset)
        fseek(binaryFile,offset,SEEK_SET);

	while(select)
	{
		printf("\n");

        //Do the acquisition, if it is ok go on.
        ecRet = AcquireMovement(1);
        if (ecRet)
        {
            select = FALSE;
            break;
        }
    }


	CloseHandle(hPort);
	fclose(binaryFile);

	return 0;

}

ERR_CODE SetupPort(char const cPort[])
{
	ERR_CODE ecRet = OK;
	pSerialCreate pParam;
	pParam = new SerialCreate;
	pParam->lBaudRate = BAUD_RATE;
	pParam->lDataBits = NUM_BITS;
	pParam->lTimeout = TIME_OUT;

	ecRet = PortInitialize(cPort, pParam);
	if (ecRet != OK)
		printf("ERROR in PortInitialize()!\n");
	else
	{
	    //Param is a struct used internally to this function to set up the handle.
	    //Since the parameters, once set, are stored in some configuration files on windows
	    //we can delete the parameters in our program, the only thing that must remain is the handler,
	    //which is a unique identifier to the port. hPort is indeed a global variable, and once it is
	    //set here, it remains alive for the whole life of the program. All the other functions that wants
	    //to use the comPort just need this handle.
		hPort = pParam->h_Port;
	}
	delete pParam;
	return ecRet;
}


ERR_CODE AcquireMovement(BOOL display)
{
	__int8 unsigned sByte = 0x73;
	int numByte = NUM_BYTE;//, MaxByte = MAX_BYTE;
	ERR_CODE ecRet = OK;
	CommPortClass* comPort = new CommPortClass;
	comPort->handlePort = hPort;
	comPort->iMaxChars = NUM_BYTE;
	comPort->binaryFile = binaryFile;

    //Start communication
    ecRet = PortWrite(hPort, sByte, numByte);
    if (ecRet)
    {
        printf("PortWrite() is failed\n");
        TestPassed = FALSE;
        CloseHandle(hPort);
        return EC_WRITE_FAIL;
    }


    //Read the incoming stream
    ecRet = PortRead(comPort);
    //The way this program behave is read until it happens a timeout event,
    //Therefore each error different from time out is a problem and the port should be
    //closed. If instead the error is timeout, everything's ok and we can keep acquiring movements.
    if (ecRet != EC_RECV_TIMEOUT)
    {
        printf("PortRead() is failed\n");
        TestPassed = FALSE;
        CloseHandle(hPort);
        return EC_READ_FAIL;
    }

    ecRet = OK;

	delete comPort;
	return ecRet;
}

__int8 getMenuItem(unsigned char mPort)
{
	__int8 ret;
	switch (mPort)
	{
	case 'A':
	case 'a': ret = MOVEUP;
			break;
	case 'B':
	case 'b': ret = MOVDOWN;
			break;
	case 'C':
	case 'c': ret = ROTOUT;
			break;
	case 'D':
	case 'd': ret = ROTIN;
			break;
	case 'E':
	case 'e': ret = HANDOPEN;
			break;
	case 'F':
	case 'f': ret = HANDCLOSE;
			break;

	case 'X':
	case 'x': ret = EXITPROG;
			break;
	default: printf("Invalid Selection\n");
	ret = NULLSELECTION;
	}
	return ret;
}




