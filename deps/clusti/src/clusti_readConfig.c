
#include "clusti.h"
#include "clusti_types.h"

#include<stdio.h>


/* could become private */
void clusti_Stitcher_readConfig(Clusti_Stitcher *stitcher,
				const char *configPath)
{
	//stitcher->pars

	FILE *fp;
	fp = fopen("../../../testdata/calibration_viewfrusta.xml", "r");
	if (fp == NULL) {
		printf("Failed to open file\n");
		return 1;
	}
	
	XML_Parser parser = XML_ParserCreate("utf-8");

	//XML_SetElementHandler(parser, start_element, end_element);
	//XML_SetCharacterDataHandler(parser, handle_data);
	//
	//memset(buff, 0, buff_size);
	//printf("strlen(buff) before parsing: %d\n", strlen(buff));
	//
	//size_t file_size = 0;
	//file_size = fread(buff, sizeof(char), buff_size, fp);
	//
	///* parse the xml */
	//if (XML_Parse(parser, buff, strlen(buff), XML_TRUE) == XML_STATUS_ERROR) {
	//	printf("Error: %s\n", XML_ErrorString(XML_GetErrorCode(parser)));
	//}
	
	fclose(fp);
	XML_ParserFree(parser);
}
