#include <string.h>
#include <stdio.h>


void get_bbb_id(char * ip, char * id)
{
	int len = strlen(ip);
	int i,j;
     
	j = 0;
	for (i=len-1; i>=0; i--)
	{
          if (ip[i] == '.')
		  break;
	  else
	  {
		  j++;
	  }
	}
        int id_len = j;
	for (i = len - 1; i >= len - id_len; i--)
	{
             id[j-1] = ip[i];
	     j--;
	}
}
