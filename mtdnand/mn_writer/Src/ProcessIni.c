#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "nand_sa_writer.h"


#define INI_BUF_SIZE    512
char iniBuf[INI_BUF_SIZE];
int  buffer_current = 0, buffer_end = 0;    // position to buffer iniBuf

INI_INFO_T Ini_Writer;


/*-----------------------------------------------------------------------------
 * To read one string line from file fd_ini and save it to Cmd.
 * Return:
 *      Successful  : OK
 *      Fail        : Error
 *---------------------------------------------------------------------------*/
int readLine (FILE *fd_ini, char *Cmd)
{
    int nReadLen, i_cmd;

    i_cmd = 0;

    while (1)
    {
        //--- parse INI file in buffer iniBuf[] that read in at previous fsReadFile().
        while(buffer_current < buffer_end)
        {
            if (iniBuf[buffer_current] == 0x0D)
            {
                // DOS   use 0x0D 0x0A as end of line;
                // Linux use 0x0A as end of line;
                // To support both DOS and Linux, we treat 0x0A as real end of line and ignore 0x0D.
                buffer_current++;
                continue;
            }
            else if (iniBuf[buffer_current] == 0x0A)   // Found end of line
            {
                Cmd[i_cmd] = 0;     // end of string
                buffer_current++;
                return Successful;
            }
            else
            {
                Cmd[i_cmd] = iniBuf[buffer_current];
                buffer_current++;
                i_cmd++;
            }
        }

        //--- buffer iniBuf[] is empty here. Try to read more data from file to buffer iniBuf[].

        // no more data to read since previous fsReadFile() cannot read buffer full
        if ((buffer_end < INI_BUF_SIZE) && (buffer_end > 0))
        {
            if (i_cmd > 0)
            {
                // return last line of INI file that without end of line
                Cmd[i_cmd] = 0;     // end of string
                return Successful;
            }
            else
            {
                Cmd[i_cmd] = 0;     // end of string to clear Cmd
                return Fail;
            }
        }
        else
        {
            nReadLen = fread(iniBuf, 1, INI_BUF_SIZE, fd_ini);
            if (nReadLen == 0)
            {
                Cmd[i_cmd] = 0;     // end of string to clear Cmd
                return Fail;        // error or end of file
            }
            buffer_current = 0;
            buffer_end = nReadLen;
        }
    }   // end of while (1)
}


/*-----------------------------------------------------------------------------
 * To parse INI file and store configuration to global variable Ini_Writer.
 * Return:
 *      Successful  : OK
 *      Fail        : Error
 *---------------------------------------------------------------------------*/
int ProcessINI(char *fileName)
{
    char Cmd[256];
    int  status;
    FILE *fd_ini;

    //--- initial default value
    Ini_Writer.NandLoader[0] = 0;
    Ini_Writer.Logo[0] = 0;
    Ini_Writer.ExecFile[0] = 0;
    Ini_Writer.ExecFileNum = 0;
    Ini_Writer.ExecAddress = 0;

    //--- open INI file
    fd_ini = fopen(fileName, "r");
    if (fd_ini == NULL)
    {
        printf("ERROR: fail to open INI file %s !\n", fileName);
        return Fail;
    }

    //--- parse INI file
    do {
        status = readLine(fd_ini, Cmd);
        if (status < 0)     // read file error. Coulde be end of file.
            break;
NextMark2:
        if (strcmp(Cmd, "[NandLoader File Name]") == 0)
        {
            do {
                status = readLine(fd_ini, Cmd);
                if (status < 0)
                    break;          // use default value since error code from NVTFAT. Coulde be end of file.
                else if (Cmd[0] == 0)
                    continue;       // skip empty line
                else if ((Cmd[0] == '/') && (Cmd[1] == '/'))
                    continue;       // skip comment line
                else if (Cmd[0] == '[')
                    goto NextMark2; // use default value since no assign value before next keyword
                else
                {
                    if (strlen(Cmd) > 511)
                    {
                        printf("ERROR: NandLoader File Name too long !! It must < 512 !!\n");
                        fclose(fd_ini);
                        return Fail;
                    }
                    strcpy(Ini_Writer.NandLoader, Cmd);
                    break;
                }
            } while (1);
        }

        else if (strcmp(Cmd, "[Logo File Name]") == 0)
        {
            do {
                status = readLine(fd_ini, Cmd);
                if (status < 0)
                    break;          // use default value since error code from NVTFAT. Coulde be end of file.
                else if (Cmd[0] == 0)
                    continue;       // skip empty line
                else if ((Cmd[0] == '/') && (Cmd[1] == '/'))
                    continue;       // skip comment line
                else if (Cmd[0] == '[')
                    goto NextMark2; // use default value since no assign value before next keyword
                else
                {
                    if (strlen(Cmd) > 511)
                    {
                        printf("ERROR: Logo File Name too long !! It must < 512 !!\n");
                        fclose(fd_ini);
                        return Fail;
                    }
                    strcpy(Ini_Writer.Logo, Cmd);
                    break;
                }
            } while (1);
        }

        else if (strcmp(Cmd, "[Execute File Name]") == 0)
        {
            do {
                status = readLine(fd_ini, Cmd);
                if (status < 0)
                    break;          // use default value since error code from NVTFAT. Coulde be end of file.
                else if (Cmd[0] == 0)
                    continue;       // skip empty line
                else if ((Cmd[0] == '/') && (Cmd[1] == '/'))
                    continue;       // skip comment line
                else if (Cmd[0] == '[')
                    goto NextMark2; // use default value since no assign value before next keyword
                else
                {
                    if (strlen(Cmd) > 511)
                    {
                        printf("ERROR: Exec File Name too long !! It must < 512 !!\n");
                        fclose(fd_ini);
                        return Fail;
                    }
                    strcpy(Ini_Writer.ExecFile, Cmd);
                    break;
                }
            } while (1);
        }
        
        else if (strcmp(Cmd, "[Execute Image Number]") == 0)
        {
            do {
                status = readLine(fd_ini, Cmd);
                if (status < 0)
                    break;          // use default value since error code from NVTFAT. Coulde be end of file.
                else if (Cmd[0] == 0)
                    continue;       // skip empty line
                else if ((Cmd[0] == '/') && (Cmd[1] == '/'))
                    continue;       // skip comment line
                else if (Cmd[0] == '[')
                    goto NextMark2; // use default value since no assign value before next keyword
                else
                {
                    Ini_Writer.ExecFileNum = atoi(Cmd);
                    break;
                }
            } while (1);
        }

        else if (strcmp(Cmd, "[Execute Image Address]") == 0)
        {
            do {
                status = readLine(fd_ini, Cmd);
                if (status < 0)
                    break;          // use default value since error code from NVTFAT. Coulde be end of file.
                else if (Cmd[0] == 0)
                    continue;       // skip empty line
                else if ((Cmd[0] == '/') && (Cmd[1] == '/'))
                    continue;       // skip comment line
                else if (Cmd[0] == '[')
                    goto NextMark2; // use default value since no assign value before next keyword
                else
                {
                    Ini_Writer.ExecAddress = atoi(Cmd);
                    break;
                }
            } while (1);
        }
        
    } while (status >= 0);  // keep parsing INI file

    fclose(fd_ini);

    //--- show final configuration
    printf("Process %s file ...\n", fileName);
    printf("    NnandLoader     = %s\n", Ini_Writer.NandLoader);
    printf("    Logo            = %s\n", Ini_Writer.Logo);
    printf("    Exec File       = %s\n", Ini_Writer.ExecFile);
    printf("    Exec File Num   = %d\n", Ini_Writer.ExecFileNum);
    printf("    Exec Address    = 0x%08X\n", Ini_Writer.ExecAddress);
    return Successful;
}
