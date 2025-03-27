/* BNL */
/* Huijuan: 02-10-2014*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <dbAccess.h>
#include <dbDefs.h>
#include <dbFldTypes.h>
#include <registryFunction.h>
#include <aSubRecord.h>
#include <waveformRecord.h>
#include <epicsExport.h>
#include <epicsTime.h>

#define MASS_STRING	10
#define VAL_STRING	20
#define BAR_MASS_MAX	100
#define PRFL_MASS_MAX	5000
#define MEAS_STRING	10000

int rga_subDebug = 0;

static long asubBarMeasInit(aSubRecord* pasub) 
{
	if (rga_subDebug)
        	printf("Record %s called asubBarMeasInit(%p)\n",pasub->name, (void*) pasub);

	return 0; 
}

static long asubBarMeasRawProc(aSubRecord *pasub)
{
	char *aptr, *bptr, data_all_msg;
	char massStr[MASS_STRING] = {0}, valStr[VAL_STRING] = {0}, timeStr[MASS_STRING] ={0};
	double massDbl[BAR_MASS_MAX] = {0.0}, valDbl[BAR_MASS_MAX] = {0.0};
	double startMass = 0.0;
	int measIndex = 0;
	int stopInt = 0, arrayProc = 0, arrayRst = 0, data_all_proc;
	int len, i, j;
	long timeInt = 0;
	static char measNext[MEAS_STRING];
	static char measEnd[50];
	char measStr[MEAS_STRING] = {0};

	if (rga_subDebug)
	{
		printf("Record %s called asubBarMeasRawProc(%p)\n",pasub->name, (void*) pasub);
	}

	/* Reset all buffers */
	memset(massDbl, 0, pasub->nove*sizeof(double));
	memset(valDbl, 0, pasub->novf*sizeof(double));
	memset(measStr, 0, MEAS_STRING*sizeof(char));

	/* Clear stop signal for new cycle */
	memcpy((int *)pasub->vala, &arrayRst, pasub->nova*sizeof(int));
	memcpy((int *)pasub->valb, &stopInt, pasub->novb*sizeof(int));
	memcpy((int *)pasub->valc, &arrayProc, pasub->novc*sizeof(int));	
	memcpy((long *)pasub->vald, &timeInt, pasub->novd*sizeof(long));
	
	/* Get the response with one data command */
	aptr = (char *)pasub->a;
	bptr = (char *)pasub->b;
        startMass = ((double *)pasub->c)[0];
	data_all_proc = ((int *)pasub->d)[0];

	// Check the return value of 'data all' command
	if(data_all_proc == 1)
	{
		data_all_msg = (char *)pasub->e;
		data_all_proc = 0;
		printf("test2, data compare is %d\n", strcmp(data_all_msg, "\r\n") );
		if(strcmp(data_all_msg, "\r\n"))	
			strcat(aptr, data_all_msg);
	}
			
	printf("test2\n");
	/* Check if there's value of last cycle */
	if(strcmp(measNext, "\0") != 0)
	{
		if(rga_subDebug)
		{
			printf("Measurements from last cycle is %s", measNext);
			printf("Measurements in current cycle is %s", aptr);
		}

		strcpy(aptr, strcat(measNext, aptr));
		
		if (rga_subDebug)
			printf("Measurements from start mass is %s \n", aptr);
		memset(measNext, 0, MEAS_STRING*sizeof(char));
	}
	printf("test3\n");
        /* Check if the end of the measurement is returned with data stop command */
        if(strcmp(bptr, "\0") != 0 && strcmp(measEnd, bptr) != 0)
	{
		strcpy(measEnd, bptr);
		len = strlen(bptr);

       	        if (bptr[len-1] == '!')
               	        strcat(aptr, measEnd);
                else
       	                strcat(aptr, "!");

		memset(bptr, 0, len*sizeof(char));
		if(rga_subDebug)
			printf("Response of data stop is %s, end of the measurements is %s\n", measEnd, aptr);
	}

	/* Ignore the following processing if no data returned */	
	if(strcmp(aptr, "\0")== 0)
	{
		arrayProc = 1;
		memcpy((int *)pasub->valc, &arrayProc, pasub->novc*sizeof(int));
		return (0);
	}
	else
	{
		len = strlen(aptr);	
		if (rga_subDebug)
			printf("Measurements from start mass is %s \n", aptr);
	}

	/* The first response of data command is as [/ 0/{\r\n */
	/* The format of measurement is as 92.00: 3.18468E-7,93.00: 3.16447E-7,94.00: 3.23166E-7,95.00: 3.20478E-7,96.00: 3.16447E-7,\r\n*/
	for( i = 0; i < len; i++ )
	{
		/* Check if it's the end of the cycle */	
		if(aptr[i] == '!')
		{
			stopInt = 1;
			if (rga_subDebug)
				printf("stopInt is %d\n",  stopInt);
			break;
		}
        	
		if(aptr[i] == '}' && aptr[i+1] == ']')
		{

			/* Check if it's the end of whole scan cycle */
			/* If yes, stop data polling and end the process */
			if(aptr[i+2] == '!')
			{
				stopInt = 1;
				if (rga_subDebug)
					printf("stopInt is %d\n",  stopInt);

				if(rga_subDebug)
					printf("This is the last cycle\n");
				break;
			}
			else
			{
				strncpy(measNext, &aptr[i+2], len-i-3);    
				if(rga_subDebug)
					printf("End of this cycle, next measure is %s", measNext);
				break;
			}
		}

		/* Check if it's the separation for each mass use, measurement group */
		if(aptr[i] == '[' || aptr[i] == ',' || aptr[i] == '{' || aptr[i] == '\r' || aptr[i] == '\n')
			i++;
	
		/* Check if it's the measurement time for each scan */
		if(aptr[i] == '/')
		{
			j = 0;
			while( aptr[i+1+j] != '/')
				j++;
			
			strncpy(timeStr, &aptr[i+1], j);
			timeStr[j] = '\0';
			timeInt = atoi(timeStr);
			i = i+1+j;
			if (rga_subDebug)
			{
				printf("Time end is %c, j= %d, time string is %s ", aptr[i], j, timeStr); 
				printf("Time is %ld\n", timeInt);
			}
		}
				
		/* Check if it's the mass use */	
		/* Check if the mass is single figure */
		if(aptr[i] >= '0' && aptr[i] <= '9' && aptr[i+1] == '.' && aptr[i+4] == ':')	
		{
			strncpy(massStr, &aptr[i], 4);
			massStr[5] = '\0';
			massDbl[measIndex] = atof(massStr);
			i = i+5;
			if(startMass == atof(massStr))
				arrayRst = 1;

			if (rga_subDebug)
			{
				printf("Mass end is %c, mass string is %s ", aptr[i], massStr); 
				printf("Index is %d, mass is %f\t", measIndex, massDbl[measIndex]);
			}		
		}

		/* Check if the mass is double figure */
		if(aptr[i] >= '0' && aptr[i] <= '9' && aptr[i+2] == '.' && aptr[i+5] == ':') 
		{
			strncpy(massStr, &aptr[i], 5);
			massStr[6] = '\0';
			massDbl[measIndex] = atof(massStr);
			i = i+6;
			
			if (rga_subDebug)
			{
				printf("Mass end is %c, mass string is %s ", aptr[i], massStr); 
				printf("Index is %d, mass is %f\t", measIndex, massDbl[measIndex]);
			}		
		}

		/* Check if the mass is three figures */
		if(aptr[i] == '1' && aptr[i+3] == '.' && aptr[i+6] == ':')
		{
			strncpy(massStr, &aptr[i], 6);
			massStr[7] = '\0';
			massDbl[measIndex] = atof(massStr);
			i = i+7;
			if (rga_subDebug)
			{
				printf("Mass end is %c, mass string is %s ", aptr[i], massStr); 
				printf("Index is %d, mass is %f\t", measIndex, massDbl[measIndex]);
			}		
		}
		
		/* Check if it's the measured value */	
        	if((aptr[i] == ' '||aptr[i] == '-') && (aptr[i+1] >= '0' && aptr[i+1] <= '9' ))
		{
			j = 1;
			while(aptr[i+j] != ',')
				j++;
			strncpy(valStr, &aptr[i], j);
			valStr[j] = '\0';
			valDbl[measIndex] = atof(valStr);
			if(valDbl[measIndex] < 0)
				valDbl[measIndex] = 0;
			i = i+j;
			if (rga_subDebug)
			{
				printf("Measurement end is %c, j is %d, val string is %s ", aptr[i], j, valStr); 
				printf("The val is %e\n", valDbl[measIndex]);
			}
			measIndex++;	
		}
		
	}

	memcpy((int *)pasub->vala, &arrayRst, pasub->nova*sizeof(int));
	memcpy((int *)pasub->valb, &stopInt, pasub->novb*sizeof(int));
	memcpy((int *)pasub->valc, &arrayProc, pasub->novc*sizeof(int));
	memcpy((long *)pasub->vald, &timeInt, pasub->novd*sizeof(long));
	memcpy((double *)pasub->vale, massDbl, pasub->nove*sizeof(double));
	memcpy((double *)pasub->valf, valDbl, pasub->novf*sizeof(double));
	memcpy((int *)pasub->valg, &measIndex, pasub->novg*sizeof(int));
	memcpy((int *)pasub->valh, &measIndex, pasub->novh*sizeof(int));
	memcpy((int *)pasub->vali, &data_all_proc, pasub->novi*sizeof(int));

	/* Reset all buffers */
	memset(aptr, 0, len*sizeof(char));
	memset(bptr, 0, len*sizeof(char));
	memset(massDbl, 0, pasub->nove*sizeof(double));
	memset(valDbl, 0, pasub->novf*sizeof(double));

	pasub->pact = FALSE;

	return(0);
}

static long asubBarMeasWfProc(aSubRecord *pasub)
{
        DBLINK *plink = NULL;
        DBADDR *paddr = NULL;
        waveformRecord *pwf = NULL;
        double massArray[BAR_MASS_MAX], valArray[BAR_MASS_MAX];
        int i, nPoints;

        nPoints = pasub->nea;

        if (rga_subDebug)
                printf("Record %s called asubPrflMeasWfProc(%p)\n",pasub->name, (void*) pasub);

        /* Reset all buffers */
        memset(massArray, 0, nPoints*sizeof(double));
        memset(valArray, 0, nPoints*sizeof(double));

        /* Get the measurement pairs and start mass value*/
        memcpy(massArray, (double *)pasub->a, nPoints*sizeof(double));
        memcpy(valArray, (double *)pasub->b, nPoints*sizeof(double));

        if(rga_subDebug)        
        {
                for( i=0; i < nPoints; i++)
                {
                        printf("%f:%e\t",massArray[i], valArray[i]);
                }
        }

        /* Mass array holding the measurement mass readback till now */
       plink = &pasub->outa;
        if(DB_LINK != plink->type)
                return -1;
        paddr = (DBADDR *)plink->value.pv_link.pvt;
        pwf = (waveformRecord *)paddr->precord;
        pwf->nelm = nPoints;
        pwf->nord = nPoints;
        memcpy((double *)pasub->vala, massArray, nPoints*sizeof(double));

        /* Pressure array holding the measurement pressure readback till now */
        plink = &pasub->outb;
        if(DB_LINK != plink->type)
                return -1;
        paddr = (DBADDR *)plink->value.pv_link.pvt;
        pwf = (waveformRecord *)paddr->precord;
        pwf->nelm = nPoints;
        pwf->nord = nPoints;
        memcpy((double *)pasub->valb, valArray, nPoints*sizeof(double));

       return (0);	
}

static long asubBarDispInit(aSubRecord* pasub) 
{
	if (rga_subDebug)
        	printf("Record %s called asubBarDispInit(%p)\n",pasub->name, (void*) pasub);

	return 0; 
}

static long asubBarDispProc(aSubRecord *pasub)
{
	int nPos, nNORD, nPoints, nStat;
	double massDisp[BAR_MASS_MAX], valDisp[BAR_MASS_MAX];
	double massWf[BAR_MASS_MAX], massBuf[BAR_MASS_MAX];
	double valWf[BAR_MASS_MAX], valBuf[BAR_MASS_MAX];
	double posDisp[BAR_MASS_MAX];
	int i, *temp;
	DBLINK *plink = NULL;
	DBADDR *paddr = NULL;
	waveformRecord *pwf = NULL;

	if (rga_subDebug)
        	printf("Record %s called asubBarDispProc(%p)\n",pasub->name, (void*) pasub);

	nNORD = pasub->nea;
	nPos = pasub->neb;
	temp = (int *)pasub->e;
	nPoints = temp[0];
	temp = (int *)pasub->f;
	nStat = temp[0];
/*	memcpy(&nPoints, (double *)pasub->e, sizeof(double));*/

	if(rga_subDebug)
		printf("NORD, nPos, nPoints, nStat are %d, %d, %d, %d\n", nNORD, nPos, nPoints, nStat);

	/* Reset all buffers */
	memset(massDisp, 0, nPoints*sizeof(double));
	memset(valDisp, 0, nPoints*sizeof(double));
	memset(massWf, 0, nPoints*sizeof(double));
	memset(valWf, 0, nPoints*sizeof(double));
	memset(massBuf, 0, nPoints*sizeof(double));
	memset(valBuf, 0, nPoints*sizeof(double));
	memset(posDisp, 0, nPoints*sizeof(double));

	memcpy(massWf, (double *)pasub->a, nNORD*sizeof(double));
	memcpy(massBuf, (double *)pasub->b, nPos*sizeof(double));
	memcpy(valWf, (double *)pasub->c, nNORD*sizeof(double));
	memcpy(valBuf, (double *)pasub->d, nPos*sizeof(double));

	if(nNORD == nPoints && nStat != 17)
	{
		memcpy(massDisp, (double *)pasub->a, nPoints*sizeof(double));
		for(i=0; i<nPos; i++)
			valDisp[i] = valBuf[i];
		for(i=nPos; i<nPoints; i++)
			valDisp[i] = valWf[i];		
	}
	else 
	{
		memcpy(massDisp, (double *)pasub->b, nPos*sizeof(double));
		memcpy(valDisp, (double *)pasub->d, nPos*sizeof(double));
        }

	posDisp[nPos-1] = 1e-8;

	plink = &pasub->outa;
	if(DB_LINK != plink->type)
		return -1;
	paddr = (DBADDR *)plink->value.pv_link.pvt;
	pwf = (waveformRecord *)paddr->precord;
	pwf->nelm = nPoints;
	pwf->nord = nPoints;
	memcpy((double *)pasub->vala, massDisp, nPoints*sizeof(double));

	plink = &pasub->outb;
	if(DB_LINK != plink->type)
        	return -1;
	paddr = (DBADDR *)plink->value.pv_link.pvt;
	pwf = (waveformRecord *)paddr->precord;
	pwf->nelm = nPoints;
	pwf->nord = nPoints;
	memcpy((double *)pasub->valb, valDisp, nPoints*sizeof(double));

	plink = &pasub->outc;
	if(DB_LINK != plink->type)
        	return -1;
	paddr = (DBADDR *)plink->value.pv_link.pvt;
	pwf = (waveformRecord *)paddr->precord;
	pwf->nelm = nPoints;
	pwf->nord = nPoints;
	memcpy((double *)pasub->valc, posDisp, nPoints*sizeof(double));
	
	return (0);
}

static long asubPrflMeasInit(aSubRecord* pasub) 
{
	if (rga_subDebug)
        	printf("Record %s called asubPrflMeasInit(%p)\n",pasub->name, (void*) pasub);

	return 0; 
}

static long asubPrflMeasRawProc(aSubRecord *pasub)
{
	char *aptr, *bptr, *data_all_msg;
	char massStr[MASS_STRING] = {0}, valStr[VAL_STRING] = {0}, timeStr[MASS_STRING] ={0};
	double massDbl[PRFL_MASS_MAX] = {0.0}, valDbl[PRFL_MASS_MAX] = {0.0};
	double startMass = 0.0;
	int measIndex = 0;
	int stopInt = 0, arrayProc = 0, arrayRst = 0, data_all_proc = 0;
	int len, i, j;	
	long timeInt = 0;
	static char measNext[MEAS_STRING];
	static char measEnd[50];
	char measStr[MEAS_STRING] = {0};
	
	if (rga_subDebug)
	{
		printf("Record %s called asubPrflMeasRawProc(%p)\n",pasub->name, (void*) pasub);
	}

	/* Reset all buffers */
	memset(massDbl, 0, pasub->nove*sizeof(double));
	memset(valDbl, 0, pasub->novf*sizeof(double));
	memset(measStr, 0, MEAS_STRING*sizeof(char));

	/* Clear stop signal for new cycle */
	memcpy((int *)pasub->vala, &arrayRst, pasub->nova*sizeof(int));
	memcpy((int *)pasub->valb, &stopInt, pasub->novb*sizeof(int));
        memcpy((int *)pasub->valc, &arrayProc, pasub->novc*sizeof(int));        
        memcpy((long *)pasub->vald, &timeInt, pasub->novd*sizeof(long));
	
	/* Get the response with one data command */
	aptr = (char *)pasub->a;
	bptr = (char *)pasub->b;
	startMass = ((double *)pasub->c)[0];
	data_all_proc = ((int *)pasub->d)[0];

	// Check the return value of 'data all' command
	if(data_all_proc == 1)
	{
		data_all_msg = (char *)pasub->e;
		data_all_proc = 0;
		if(strcmp(data_all_msg, "\r\n"))
			strcat(aptr, data_all_msg);
	}		

	/* Check if there's value of last cycle */
	if(strcmp(measNext, "\0") != 0)
	{
		if(rga_subDebug)
		{
			printf("Measurements from last cycle is %s", measNext);
			printf("Measurements in current cycle is %s", aptr);
		}

		strcpy(aptr, strcat(measNext, aptr));
		
		if (rga_subDebug)
			printf("Measurements from start mass is %s \n", aptr);
		memset(measNext, 0, MEAS_STRING*sizeof(char));
	}

        /* Check if the end of the measurement is returned with data stop command */
        if(strcmp(bptr, "\0") != 0 && strcmp(measEnd, bptr) != 0)
	{
		strcpy(measEnd, bptr);
		len = strlen(bptr);

       	        if (bptr[len-1] == '!')
               	        strcat(aptr, measEnd);
                else
       	                strcat(aptr, "!");

		memset(bptr, 0, len*sizeof(char));
		if(rga_subDebug)
			printf("Response of data stop is %s, end of the measurements is %s\n", measEnd, aptr);
	}

	/* Ignore the following processing if no data returned */	
	if(strcmp(aptr, "\0")== 0)
	{
		arrayProc = 1;
		memcpy((int *)pasub->valc, &arrayProc, pasub->novc*sizeof(int));		
		return (0);
	}
	else
	{
		len = strlen(aptr);	
		if (rga_subDebug)
			printf("Measurements from start mass is %s \n", aptr);
	}

	/* The first response of data command is as [/ 0/{\r\n */
	/* The format of measurement is as 92.00: 3.18468E-7,93.00: 3.16447E-7,94.00: 3.23166E-7,95.00: 3.20478E-7,96.00: 3.16447E-7,\r\n*/
	for( i = 0; i < len; i++ )
	{
		/* Check if it's the end of the cycle */	
		if(aptr[i] == '!')
		{
			stopInt = 1;
			if (rga_subDebug)
				printf("stopInt is %d\n",  stopInt);
			break;
		}
        	
		if(aptr[i] == '}' && aptr[i+1] == ']')
		{

			/* Check if it's the end of whole scan cycle */
			/* If yes, stop data polling and end the process */
			if(aptr[i+2] == '!')
			{
				stopInt = 1;
				if (rga_subDebug)
					printf("stopInt is %d\n",  stopInt);

				if(rga_subDebug)
					printf("This is the last cycle\n");
				break;
			}
			else
			{
				strncpy(measNext, &aptr[i+2], len-i-3);    
				if(rga_subDebug)
					printf("End of this cycle, next measure is %s", measNext);
				break;
			}
		}

		/* Check if it's the separation for each mass use, measurement group */
		if(aptr[i] == '[' || aptr[i] == ',' || aptr[i] == '{' || aptr[i] == '\r' || aptr[i] == '\n')
			i++;

		/* Check if it's the measurement time for each scan */
		if(aptr[i]  == '/')
		{
			j = 0;
			while( aptr[i+1+j] != '/')
				j++;
			
			strncpy(timeStr, &aptr[i+1], j);
			timeStr[j] = '\0';
			timeInt = atoi(timeStr);
			i = i+1+j;
			if (rga_subDebug)
			{
				printf("Time end is %c, j= %d, time string is %s ", aptr[i], j, timeStr); 
				printf("Time is %ld\n", timeInt);
			}
		}
				
		/* Check if it's the mass use */	
		/* Check if the mass is single figure */
		if(aptr[i] >= '0' && aptr[i] <= '9' && aptr[i+1] == '.' && aptr[i+4] == ':')	
		{
			strncpy(massStr, &aptr[i], 4);
			massStr[5] = '\0';
			massDbl[measIndex] = atof(massStr);
			i = i+5;
			if(startMass == atof(massStr))
                                arrayRst = 1;

			if (rga_subDebug)
			{
				printf("Mass end is %c, mass string is %s ", aptr[i], massStr); 
				printf("Index is %d, mass is %f\t", measIndex, massDbl[measIndex]);
			}		
		}

		/* Check if the mass is double figure */
		if(aptr[i] >= '0' && aptr[i] <= '9' && aptr[i+2] == '.' && aptr[i+5] == ':') 
		{
			strncpy(massStr, &aptr[i], 5);
			massStr[6] = '\0';
			massDbl[measIndex] = atof(massStr);
			i = i+6;
			
			if (rga_subDebug)
			{
				printf("Mass end is %c, mass string is %s ", aptr[i], massStr); 
				printf("Index is %d, mass is %f\t", measIndex, massDbl[measIndex]);
			}		
		}

		/* Check if the mass is three figures */
		if(aptr[i] == '1' && aptr[i+3] == '.' && aptr[i+6] == ':')
		{
			strncpy(massStr, &aptr[i], 6);
			massStr[7] = '\0';
			massDbl[measIndex] = atof(massStr);
			i = i+7;
			if (rga_subDebug)
			{
				printf("Mass end is %c, mass string is %s ", aptr[i], massStr); 
				printf("Index is %d, mass is %f\t", measIndex, massDbl[measIndex]);
			}		
		}
		
		/* Check if it's the measured value */	
        	if((aptr[i] == ' '||aptr[i] == '-') && (aptr[i+1] >= '0' && aptr[i+1] <= '9' ))
		{
			j = 1;
			while(aptr[i+j] != ',')
				j++;
			strncpy(valStr, &aptr[i], j);
			valStr[j] = '\0';
			valDbl[measIndex] = atof(valStr);
			if(valDbl[measIndex] < 0)
				valDbl[measIndex] = 0;
			i = i+j;
			if (rga_subDebug)
			{
				printf("Measurement end is %c, j is %d, val string is %s ", aptr[i], j, valStr); 
				printf("The val is %e\n", valDbl[measIndex]);
			}
			measIndex++;	
		}
		
	}

	memcpy((int *)pasub->vala, &arrayRst, pasub->nova*sizeof(int));
        memcpy((int *)pasub->valb, &stopInt, pasub->novb*sizeof(int));
        memcpy((int *)pasub->valc, &arrayProc, pasub->novc*sizeof(int));
        memcpy((long *)pasub->vald, &timeInt, pasub->novd*sizeof(long));
        memcpy((double *)pasub->vale, massDbl, pasub->nove*sizeof(double));
        memcpy((double *)pasub->valf, valDbl, pasub->novf*sizeof(double));
        memcpy((int *)pasub->valg, &measIndex, pasub->novg*sizeof(int));
        memcpy((int *)pasub->valh, &measIndex, pasub->novh*sizeof(int));
	memcpy((int *)pasub->vali, &data_all_proc, pasub->novi*sizeof(int));

	/* Reset all buffers */
	memset(aptr, 0, len*sizeof(char));
	memset(bptr, 0, len*sizeof(char));
	memset(massDbl, 0, pasub->nove*sizeof(double));
	memset(valDbl, 0, pasub->novf*sizeof(double));

	pasub->pact = FALSE;

	return(0);
}

static long asubPrflMeasWfProc(aSubRecord *pasub)
{
        DBLINK *plink = NULL;
        DBADDR *paddr = NULL;
        waveformRecord *pwf = NULL;
	double massArray[PRFL_MASS_MAX], valArray[PRFL_MASS_MAX];
        int i, nPoints;

	nPoints = pasub->nea;

	if (rga_subDebug)
        	printf("Record %s called asubPrflMeasWfProc(%p)\n",pasub->name, (void*) pasub);

	/* Reset all buffers */
	memset(massArray, 0, nPoints*sizeof(double));
	memset(valArray, 0, nPoints*sizeof(double));

	/* Get the measurement pairs and start mass value*/
	memcpy(massArray, (double *)pasub->a, nPoints*sizeof(double));
	memcpy(valArray, (double *)pasub->b, nPoints*sizeof(double));

	if(rga_subDebug)
	{
		for( i=0; i < nPoints; i++)
		{
			printf("%f:%e\t",massArray[i], valArray[i]);
		}
	}
	
	/* Mass array holding the measurement mass readback till now */
       plink = &pasub->outa;
        if(DB_LINK != plink->type)
 		return -1;
        paddr = (DBADDR *)plink->value.pv_link.pvt;
        pwf = (waveformRecord *)paddr->precord;
        pwf->nelm = nPoints;
        pwf->nord = nPoints;
        memcpy((double *)pasub->vala, massArray, nPoints*sizeof(double));

	/* Pressure array holding the measurement pressure readback till now */
        plink = &pasub->outb;
        if(DB_LINK != plink->type)
                return -1;
        paddr = (DBADDR *)plink->value.pv_link.pvt;
        pwf = (waveformRecord *)paddr->precord;
        pwf->nelm = nPoints;
        pwf->nord = nPoints;
        memcpy((double *)pasub->valb, valArray, nPoints*sizeof(double));

        return (0);	
}

static long asubPrflDispInit(aSubRecord* pasub) 
{
	if (rga_subDebug)
        	printf("Record %s called asubPrflDispInit(%p)\n",pasub->name, (void*) pasub);

	return 0; 
}

static long asubPrflDispProc(aSubRecord *pasub)
{
	int nPos, nNORD, nPoints, nStat;
	double massDisp[PRFL_MASS_MAX], valDisp[PRFL_MASS_MAX];
	double massWf[PRFL_MASS_MAX], massBuf[PRFL_MASS_MAX];
	double valWf[PRFL_MASS_MAX], valBuf[PRFL_MASS_MAX];
	double posDisp[PRFL_MASS_MAX];
	int i, *temp;
	DBLINK *plink = NULL;
	DBADDR *paddr = NULL;
	waveformRecord *pwf = NULL;

	if (rga_subDebug)
        	printf("Record %s called asubPrflDispProc(%p)\n",pasub->name, (void*) pasub);

	nNORD = pasub->nea;
	nPos = pasub->neb;
	temp = (int *)pasub->e;
	nPoints = temp[0];
	temp = (int *)pasub->f;
	nStat = temp[0];
/*	memcpy(&nPoints, (double *)pasub->e, sizeof(double));*/

	if(rga_subDebug)
		printf("NORD, nPos, nPoints, nStat are %d, %d, %d, %d\n", nNORD, nPos, nPoints, nStat);

	/* Reset all buffers */
	memset(massDisp, 0, nPoints*sizeof(double));
	memset(valDisp, 0, nPoints*sizeof(double));
	memset(massWf, 0, nPoints*sizeof(double));
	memset(valWf, 0, nPoints*sizeof(double));
	memset(massBuf, 0, nPoints*sizeof(double));
	memset(valBuf, 0, nPoints*sizeof(double));
	memset(posDisp, 0, nPoints*sizeof(double));

	memcpy(massWf, (double *)pasub->a, nNORD*sizeof(double));
	memcpy(massBuf, (double *)pasub->b, nPos*sizeof(double));
	memcpy(valWf, (double *)pasub->c, nNORD*sizeof(double));
	memcpy(valBuf, (double *)pasub->d, nPos*sizeof(double));

	if(nNORD == nPoints && nStat != 17)
	{
		memcpy(massDisp, (double *)pasub->a, nPoints*sizeof(double));
		for(i=0; i<nPos; i++)
			valDisp[i] = valBuf[i];
		for(i=nPos; i<nPoints; i++)
			valDisp[i] = valWf[i];		
	}
	else 
	{
		memcpy(massDisp, (double *)pasub->b, nPos*sizeof(double));
		memcpy(valDisp, (double *)pasub->d, nPos*sizeof(double));
        }

	posDisp[nPos-1] = 1e-8;

	plink = &pasub->outa;
	if(DB_LINK != plink->type)
		return -1;
	paddr = (DBADDR *)plink->value.pv_link.pvt;
	pwf = (waveformRecord *)paddr->precord;
	pwf->nelm = nPoints;
	pwf->nord = nPoints;
	memcpy((double *)pasub->vala, massDisp, nPoints*sizeof(double));

	plink = &pasub->outb;
	if(DB_LINK != plink->type)
        	return -1;
	paddr = (DBADDR *)plink->value.pv_link.pvt;
	pwf = (waveformRecord *)paddr->precord;
	pwf->nelm = nPoints;
	pwf->nord = nPoints;
	memcpy((double *)pasub->valb, valDisp, nPoints*sizeof(double));

	plink = &pasub->outc;
	if(DB_LINK != plink->type)
        	return -1;
	paddr = (DBADDR *)plink->value.pv_link.pvt;
	pwf = (waveformRecord *)paddr->precord;
	pwf->nelm = nPoints;
	pwf->nord = nPoints;
	memcpy((double *)pasub->valc, posDisp, nPoints*sizeof(double));
	
	return (0);
}

static long asubMIDMeasInit(aSubRecord* pasub) 
{
	if (rga_subDebug)
        	printf("Record %s called asubMIDMeasInit(%p)\n",pasub->name, (void*) pasub);

	return 0; 
}

static long asubMIDMeasRawProc(aSubRecord *pasub)
{
	char *aptr, *bptr;
	char massStr[MASS_STRING] = {0}, valStr[VAL_STRING] = {0};
	int mass1 = 0, mass2 = 0, mass3 = 0, mass4 = 0, mass5 = 0, mass = 0;
	int mass6 = 0, mass7 = 0, mass8 = 0, mass9 = 0;
	int mass1Proc = 0, mass2Proc = 0, mass3Proc = 0, mass4Proc = 0, mass5Proc = 0;
	int mass6Proc = 0, mass7Proc = 0, mass8Proc = 0, mass9Proc = 0;

	double mass1Val = 0.0, mass2Val = 0.0, mass3Val = 0.0, mass4Val = 0.0, mass5Val = 0.0, massVal = 0.0;
	double mass6Val = 0.0, mass7Val = 0.0, mass8Val = 0.0, mass9Val = 0.0;
	int stopInt = 0, data_all_proc = 0;
	int len, i, j,  masstemp;	

	if (rga_subDebug)
	{
		printf("Record %s called asubMIDMeasRawProc(%p)\n",pasub->name, (void*) pasub);
	}

	/* Clear stop signal for new cycle */
	memcpy((int *)pasub->vala, &stopInt, pasub->nova*sizeof(int));
	memcpy((int *)pasub->valb, &mass1Proc, pasub->novb*sizeof(int));
	memcpy((int *)pasub->valc, &mass2Proc, pasub->novc*sizeof(int));
	memcpy((int *)pasub->vald, &mass3Proc, pasub->novd*sizeof(int));
	memcpy((int *)pasub->vale, &mass4Proc, pasub->nove*sizeof(int));
	memcpy((int *)pasub->valf, &mass5Proc, pasub->novf*sizeof(int));
	memcpy((int *)pasub->vall, &mass6Proc, pasub->novl*sizeof(int));
        memcpy((int *)pasub->valm, &mass7Proc, pasub->novm*sizeof(int));
        memcpy((int *)pasub->valn, &mass8Proc, pasub->novn*sizeof(int));
        memcpy((int *)pasub->valo, &mass9Proc, pasub->novo*sizeof(int));

	/* Get the response with one data command */
	aptr = (char *)pasub->a;
	if (rga_subDebug)
		printf("Measurements is %s \n", aptr);
	
	masstemp = ((int *)pasub->b)[0];
	if (masstemp != 0)
		mass1 = masstemp;
	masstemp = ((int *)pasub->c)[0];
	if (masstemp != 0)
               	mass2 = masstemp;
	masstemp = ((int *)pasub->d)[0];
       	if (masstemp != 0)
		mass3 = masstemp;
	masstemp = ((int *)pasub->e)[0];
        if (masstemp != 0)
                mass4 = masstemp;
	masstemp = ((int *)pasub->f)[0];
        if (masstemp != 0)
                mass5 = masstemp;
        masstemp = ((int *)pasub->g)[0];
        if (masstemp != 0)
                mass6 = masstemp;
        masstemp = ((int *)pasub->h)[0];
        if (masstemp != 0)
                mass7 = masstemp;
        masstemp = ((int *)pasub->i)[0];
        if (masstemp != 0)
                mass8 = masstemp;
        masstemp = ((int *)pasub->j)[0];
        if (masstemp != 0)
                mass9 = masstemp;

	data_all_proc = ((int *)pasub->k)[0];
	if(data_all_proc == 1)
	{
		bptr = (char *)pasub->b;
		data_all_proc = 0;
		if(strcmp(bptr, "\r\n"))
			strcat(aptr, bptr);
	}		

	/* Ignore the following processing if no data returned */	
	len = strlen(aptr);
	if(strcmp(aptr, "\0")== 0)
	{
		return (0);
	}

	if (rga_subDebug)
		printf(" string = %s \n", aptr);

	/* The format of measurement is as [2.00: 3.18468E-7,18.00: 3.16447E-7,28.00: 3.23166E-7,]\r\n */
	/* The end of scan cycle is as !\r\n */
	for( i = 0; i < len; i++ )
	{
		/* Check if it's the end of the cycle */	
		if(aptr[i] == '!' )
		{
			stopInt = 1;
			if (rga_subDebug)
				printf("stopInt is %d\n",  stopInt);
			break;
		}

		/* Check if it's the separation for each mass use, measurement group */
		if(aptr[i] == ','|| aptr[i] == ']'|| aptr[i] == '[' || aptr[i] == '\r' || aptr[i] == '\n')
			continue;
	
		/* Check if it's the mass use */	
		/* Check if the mass is single figure */
		if(aptr[i] >= '0' && aptr[i] <= '9' && aptr[i+1] == '.' && aptr[i+4] == ':')	
		{
			strncpy(massStr, &aptr[i], 4);
			massStr[5] = '\0';
			mass = atof(massStr);
			i = i+5;
		}
		
		/* Check if the mass is double figure */
		else if(aptr[i] >= '0' && aptr[i] <= '9' && aptr[i+2] == '.' && aptr[i+5] == ':') 
		{
			strncpy(massStr, &aptr[i], 5);
			massStr[6] = '\0';
			mass = atoi(massStr);
			i = i+6;
		}

		/* Check if the mass is three figures */
		else if(aptr[i] == '1' && aptr[i+3] == '.' && aptr[i+6] == ':')
		{
			strncpy(massStr, &aptr[i], 6);
			massStr[7] = '\0';
			mass = atof(massStr);
			i = i+7;
		}

		/* Check if it's the measured value */	
        	if((aptr[i] == ' '||aptr[i] == '-') && (aptr[i+1] >= '0' && aptr[i+1] <= '9' ))
		{
			j = 1;
			while(aptr[i+j] != ',')
				j++;
			strncpy(valStr, &aptr[i], j);
			valStr[j] = '\0';
			massVal = atof(valStr);
			i = i+j;
			if (rga_subDebug)
			{
				printf("The mass and val is %d:%e\n", mass, massVal);
			}
			if(massVal < 0)
				massVal = 0;
		}
		
		if(mass == mass1)
		{
			mass1Proc = 1;
			mass1Val = massVal;
		}
		else if (mass == mass2)
		{
			mass2Proc = 1;
			mass2Val = massVal;
		}
		else if (mass == mass3)
		{
			mass3Proc = 1;
        	        mass3Val = massVal;
		}
		else if (mass == mass4)
		{
			mass4Proc = 1;
        	        mass4Val = massVal;
		}
		else if (mass == mass5)
		{
			mass5Proc = 1;
        	        mass5Val = massVal;
		}
		else if (mass == mass6)
                {
                        mass6Proc = 1;
                        mass6Val = massVal;
                }
		else if (mass == mass7)
                {
                        mass7Proc = 1;
                        mass7Val = massVal;
                }
		else if (mass == mass8)
                {
                        mass8Proc = 1;
                        mass8Val = massVal;
                }
		else if (mass == mass9)
                {
                        mass9Proc = 1;
                        mass9Val = massVal;
                }

	}
 
	memcpy((int *)pasub->vala, &stopInt, pasub->nova*sizeof(int));
	memcpy((int *)pasub->valb, &mass1Proc, pasub->novb*sizeof(int));
	memcpy((int *)pasub->valc, &mass2Proc, pasub->novc*sizeof(int));
	memcpy((int *)pasub->vald, &mass3Proc, pasub->novd*sizeof(int));
	memcpy((int *)pasub->vale, &mass4Proc, pasub->nove*sizeof(int));
	memcpy((int *)pasub->valf, &mass5Proc, pasub->novf*sizeof(int));	
	memcpy((double *)pasub->valg, &mass1Val, pasub->novg*sizeof(double));
        memcpy((double *)pasub->valh, &mass2Val, pasub->novh*sizeof(double));
        memcpy((double *)pasub->vali, &mass3Val, pasub->novi*sizeof(double));
        memcpy((double *)pasub->valj, &mass4Val, pasub->novj*sizeof(double));
        memcpy((double *)pasub->valk, &mass5Val, pasub->novk*sizeof(double));
	
	memcpy((int *)pasub->vall, &mass6Proc, pasub->novl*sizeof(int));
        memcpy((int *)pasub->valm, &mass7Proc, pasub->novm*sizeof(int));
        memcpy((int *)pasub->valn, &mass8Proc, pasub->novn*sizeof(int));
        memcpy((int *)pasub->valo, &mass9Proc, pasub->novo*sizeof(int));
	memcpy((double *)pasub->valp, &mass6Val, pasub->novp*sizeof(double));
        memcpy((double *)pasub->valq, &mass7Val, pasub->novq*sizeof(double));
        memcpy((double *)pasub->valr, &mass8Val, pasub->novr*sizeof(double));
        memcpy((double *)pasub->vals, &mass9Val, pasub->novs*sizeof(double));
	memcpy((int *)pasub->valt, &data_all_proc, pasub->novt*sizeof(int));
		
	/* Reset all buffers */
	memset(aptr, 0, len*sizeof(char));

	return(0);
}

/* Register these symbols for use by IOC code: */
epicsExportAddress(int, rga_subDebug);
epicsRegisterFunction(asubBarMeasInit);
epicsRegisterFunction(asubBarMeasRawProc);
epicsRegisterFunction(asubBarMeasWfProc);
epicsRegisterFunction(asubPrflMeasInit);
epicsRegisterFunction(asubPrflMeasRawProc);
epicsRegisterFunction(asubPrflMeasWfProc);
epicsRegisterFunction(asubMIDMeasInit);
epicsRegisterFunction(asubMIDMeasRawProc);
epicsRegisterFunction(asubBarDispInit);
epicsRegisterFunction(asubBarDispProc);
epicsRegisterFunction(asubPrflDispInit);
epicsRegisterFunction(asubPrflDispProc);

