// ShFDecoder.cpp : Defines the entry point for the console application.
// Yurchenko Volodymyr, 2010
//

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <io.h>
#include <vector>
#include <list>
#include <string>
using namespace std;

struct Tree
{
	unsigned char ch;
	Tree* left;
	Tree* right;
};

// -----------------------------------------------------------------------
// Rebuild Code Table

void RebuildTable(Tree* &root, unsigned char *table, unsigned char sequence, unsigned char code_length)
{
	static unsigned char count = 0;
	static bool first = true;
	static unsigned int temp_num;
	static char prev_length = 0;
	unsigned int shifter = 64;
	Tree* ptree = root;

	for (count; count != sequence; count++)
	{
		if (first == true)    // Have the first element
		{
			prev_length = code_length;
			for (unsigned char j = 0; j < code_length; j++)
			{
				temp_num = (temp_num << 1) | 1;
				if (!ptree->right)   // if (ptree->right == NULL)
				{
					Tree *node = new Tree;
					node->left = NULL;
					node->right = NULL;
					ptree->right = node;
					ptree = ptree->right;
				}
				else
				{
					ptree = ptree->right;
				}

			}
			first = false;
		}
		else
		{
			while (prev_length < code_length)
			{
				temp_num <<= 1;   // Put 0 at the end
				++prev_length;
			}
			while (prev_length > code_length)
			{
				temp_num >>= 1;   // Delete 0 at the end
				--prev_length;
			}
			temp_num = temp_num - 1;

			shifter = pow(2, code_length-1);
			for (unsigned char j = 0; j < code_length; j++)
			{
				if (temp_num & shifter)  //  if (temp_num & shifter == 1)
				{
					if (!ptree->right)    // if (ptree->right == NULL)
					{
						Tree *node = new Tree;
						node->left = NULL;
						node->right = NULL;
						ptree->right = node;
						ptree = ptree->right;
					}
					else
					{
						ptree = ptree->right;
					}
				}
				else
				{
					if (!ptree->left)   // if (ptree->left == NULL)
					{
						Tree *node = new Tree;
						node->left = NULL;
						node->right = NULL;
						ptree->left = node;
						ptree = ptree->left;
					}
					else
					{
						ptree = ptree->left;
					}
				}
				shifter >>= 1; // shifter / 2

			}
		}
		ptree->ch = table[count];
		ptree = root;
	}
	prev_length = code_length;
}

// -----------------------------------------------------------------------
// Decode compressed data

void Decode(Tree* &root, unsigned char* &data, unsigned int size, unsigned char &rest, unsigned short alpha)
{
	FILE *f2 = fopen("Decoded.b", "wb");
	string temp_buf;
	unsigned int shifter = 64;
	Tree* ptree;
	ptree = root;

	for (unsigned int i = 0; i < size - 1; i++)
	{
		for (shifter = 64; shifter >= 1; shifter >>= 1)
		{
			if ((unsigned int)data[i] & shifter)   //  if (data[i] & shifter == 1)
			{
				if (ptree->right)
				ptree = ptree->right;
				else
				{
					fputc(ptree->ch, f2);
					ptree = root->right;
				}
			}
			else
			{
				if (ptree->left)
				ptree = ptree->left;
				else
				{
					fputc(ptree->ch, f2);
					ptree = root->left;
				}
			}
		}
		shifter = 64;
	}
	for (unsigned int i = 0; i < 8 - rest; i++)
	{
      	if ((unsigned int)data[size - 1] & shifter)   //  if (data[i] & shifter == 1)
        {
         	if (ptree->right)
         	ptree = ptree->right;
            else
            {
            	fputc(ptree->ch, f2);
            	ptree = root->right;
            }
        }
        else
        {
         	if (ptree->left)
            ptree = ptree->left;
            else
            {
            	fputc(ptree->ch, f2);
				ptree = root->left;
            }
        }
        shifter >>= 1;
	}
	fclose(f2);
}

// -----------------------------------------------------------------------

int main(int argc, char* argv[])
{
	unsigned char *table;       // Buffer for the header table from the source file
	unsigned char *data;        // Buffer for the info from the source file
	unsigned short alpha = 0;   // The size of the alphabet of the message
	FILE *f1;    				// File for source info
	unsigned long size = 0;     // Size of source file
	unsigned int sum = 0;
	unsigned char rest = 0;
	unsigned short quantity = 0;
	unsigned char merge = 0;
	unsigned char fringe = 1;
	Tree *root;
	Tree *node = new Tree;
	node->left = NULL;
	node->right = NULL;
	root = node;

	if (argc != 3) // Проверка на корректность ввода
	{
		printf("Please, use syntax:\n <ShFDecoder.exe> br <filename>\n");
	}
	else
	{
		if ((f1 = fopen("Target.b", "rb")) == NULL)//argv[2], "rb")) == NULL) // Проверка на наличие файла  
		{
				printf("File does not exist: \"Target.b\"\n");
		}
		else
		{
			printf("Please, wait.\n");
			size = filelength(fileno(f1));                    // Counting the size
			fread(&alpha, 1, 1, f1);
			if (alpha == 0)
			{
				alpha = 256;
			}
			table = new unsigned char[alpha];                 // Allocating memory for the buffer
			fread(table, sizeof(unsigned char), alpha, f1);   // Reading the source file to buffer

			while (quantity - alpha)  // while (quantity <= alpha)
			{
				unsigned char sequence = 0;
				unsigned char code_length = 0;
				fread(&sequence, 1, 1, f1);
				fread(&code_length, 1, 1, f1);
				quantity += sequence + 1;
				RebuildTable(root, table, quantity, code_length);
				sum += 2;
			}

			data = new unsigned char[size - alpha - sum - 2];
			fread(data, sizeof(unsigned char), size - alpha - sum - 2, f1);   // Reading the source file to buffer
			fread(&rest, sizeof(unsigned char), 1, f1);
			Decode(root, data, size - alpha - sum - 2, rest, alpha);
			printf("\nDecoding completed.");

			fclose(f1);
		}
	}

	printf("\n---------------------------------\nTime : %f sec\n\a\a\a\n", (double)clock()/CLOCKS_PER_SEC);
	printf("Header size %d\n", alpha + sum + 1);
	printf("\nProgram has finished its job.\nPress any key to exit.\n");
	getchar();
	return 0;
}
