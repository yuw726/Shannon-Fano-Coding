// ShFCoder.cpp : Defines the entry point for the console application.
// Yurchenko Volodymyr, 2010
//

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <io.h>
#include <vector>
using namespace std;

// Symbol structure
typedef struct
{
	unsigned char ch;
	unsigned long prob;
	vector<unsigned char> code;
} symbol;

// -----------------------------------------------------------------------
// Sort prob table function

void Quick_sort(symbol* &b, unsigned char left, unsigned char right)
{
	unsigned char i = left, j = right;
	symbol swap;
	unsigned int t = b[(i + j) >> 1].prob;

	while (i <= j)
	{
		while (b[i].prob > t)
			i++;
		while (t > b[j].prob)
			j--;

		if (i <= j)
		{
			if ((b[i].prob < b[j].prob))
			{
				swap = b[i];
				b[i] = b[j];
				b[j] = swap;
			}

			i++;
			j--;
		}
	}

	if (left < j)
		Quick_sort(b, left, j);
	if (i < right)
		Quick_sort(b, i, right);
}

// -----------------------------------------------------------------------
// Coding function

void Coding_Shannon(symbol* &a, unsigned int left, unsigned int right, unsigned long sum)
{
   unsigned long rest = 0;     // Sum of probability in one half of the prob table
   unsigned int merge = 0;     // Number of merge character
   unsigned long fringe = 0;   // Merge probability

   if (left != right)
   if (right != 0)
   {
      for (unsigned int i = left; i <= right; i++)
      {
			rest += a[i].prob;
			if (a[i].prob > ((sum + 1) >> 1)) // Prob of symbol > sum/2, then usual table cannot be built => need handling
			{                                 // Here it is...
				a[i].code.push_back(1);
				merge = i;
				fringe = a[i].prob;
				continue;
			}
			if (rest <= ((sum + 1) >> 1))     // Symbol is in the first half of the prob table
			{
				a[i].code.push_back(1);       // Write '1'
				fringe = rest;
				merge = i;
			}
			else                              // Symbol is in the second half of the prob table
			{
				a[i].code.push_back(0);       // Write '0'
			}
		}
		if (left <= merge && merge + 1 <= right)  // Divide and handle both halfs
		{
			Coding_Shannon(a, left, merge, fringe);
			Coding_Shannon(a, merge + 1, right, sum - fringe);
		}
	}
}

// -----------------------------------------------------------------------
// Form header with the info for the further decompression

void FormHeader(FILE* f, symbol* &symb, unsigned short alpha)
{
	if (alpha == 1)
	{
		fwrite(&alpha, 1, 1, f);
		fputc(symb[0].ch, f);
		fputc(0, f);                 		// Write pairs of type: <Count, Length>
		fputc(1, f);
		printf("\nHeader size 4\n");
	}
	else
	{
		fwrite(&alpha, 1, 1, f);            // Write the number of characters, used in the code table
											// In another words - the alphabet of the message
		for (unsigned short i = 0; i < alpha; i++)
		{
			fputc(symb[i].ch, f);           // Write sequentially symbols from the code table
		}

		unsigned int count = 0;             // This will count the number of codes with the same length in succession
		unsigned int hsize = alpha;
		for (unsigned short i = 0; i < alpha; i++)
		{
			while (symb[i].code.size() == symb[i+1].code.size())
			{
				++count;                    // Here compare code length of the neighbouring symbols
				++i;                        // and count them, if codes are of the same length
			}
			fputc(count, f);                // Write pairs of type: <Count, Length>
			fputc(symb[i].code.size(), f);
			hsize += 2;
			count = 0;
		}

		printf("\nHeader size %d\n", hsize + 1);
	}
}

// -----------------------------------------------------------------------
// Write coded info into file

void Pack(symbol* &symb, unsigned long size, unsigned char* &message, unsigned short alpha)
{
	unsigned char c;                  // Char to form a byte
	unsigned char cnt = 0;
	double aver = 0.0;
	FILE *f = fopen("Target.b", "wb");

	FormHeader(f, symb, alpha);       // Put the service info

	// Here, coding the message
	for (unsigned int k = 0; k < size; k++)
	{
		for (unsigned short i = 0; i < alpha; i++)
		{
			if (symb[i].ch != message[k]) continue;
			for (vector<unsigned char>::iterator j = symb[i].code.begin(); j != symb[i].code.end(); j++)
			{
				if (*j == 1)
				{
					c = (c << 1) | 1;      // Write '1'
					++cnt;
				}
				else
				{
					++cnt;                 // Write '0'
					c <<= 1;
				}
				if (cnt == 7)              // Have a byte
				{
					aver += 8;
					cnt = 0;
					fprintf(f, "%c", c);
					c = 0;
				}
			}
		}
	}
	aver = aver + cnt + 1;
	unsigned char shift = 0;
	if (cnt != 0)
	{
		shift = 7 - cnt;
		while (cnt != 7)
		{
			c <<= 1;
			++cnt;
		}
		fprintf(f, "%c", c);
	}

	fwrite(&shift, 1, 1, f);

	printf("Average quantity of bits per symbol: %f\n", aver/((float) size));
   
	fclose(f);
}

// -----------------------------------------------------------------------

int main(int argc, char* argv[])
{
    unsigned char *message;   // Buffer for the info from the source file
    symbol symb[256];         // Array, representing symbols, their probabilities and codes
    double entropy = 0.0;     // The result for entropy
    unsigned short alpha = 0; // The size of the alphabet of the message
    FILE *f1, *f2, *f3, *f4;  // Files for source, target and service info
    unsigned long size = 0;   // Size of source file
    unsigned long sum = 0;    // Sum of the probabilities of the message

    if (argc != 3) // Проверка на корректность ввода
    {
        printf("Please, use syntax:\n <ShFCoder.exe> br <filename>\n");
    }
    else
    {
        if ((f1 = fopen(argv[2], "rb")) == NULL) // Проверка на наличие файла    
        {
            printf("File does not exist: %s\n", argv[2]);
        }
		else
		{
			printf("Please, wait...\n");
			size = filelength(fileno(f1));                    // Counting the size
			if (size == 0)
			{
				printf("File size equals zero");
				printf("\n\nTime : %f sec\n\a\a\a", (double)clock()/CLOCKS_PER_SEC);
				printf("\nProgram has finished its job.\nPress any key to exit.\n");
				getchar();
				exit(0);
			}
			message = new unsigned char[size];                // Allocating memory for the buffer
			fread(message, sizeof(unsigned char), size, f1);  // Reading the source file to buffer

			// Init parameters of the symbols
			for (unsigned short i = 0; i < 256; i++)
			{
				symb[i].prob = 0;
			}

			// Calculating the probabilities
			for (unsigned int i = 0; i < size; i++)
			{
				++symb[(int)message[i]].prob;
			}

			for (unsigned short i = 0; i < 256; i++)
			{
				if (symb[i].prob != 0)
				{
					++alpha;
					sum = sum + symb[i].prob;
				}
				symb[i].ch = i;
			}

			// Calculating entropy
			for (unsigned short i = 0; i < 256; i++)
			{
				if (symb[i].prob != 0)
				{
					entropy = entropy + ((float)symb[i].prob/(float)(size))*log((float)symb[i].prob/(float)(size))/log(2);
				}
			}
			entropy = -entropy;
			printf("Entropy: %f\n", entropy);

			printf("Alphabet: %d\n", alpha);

			Quick_sort(symb, 0, 255);
			if (alpha != 1)
			Coding_Shannon(symb, 0, alpha-1, sum);
			else symb[0].code.push_back(1);
			
			printf("\nCoding competed.\n");

			Pack(symb, size, message, alpha);
			printf("\nCompressing competed.\n");

			f2 = fopen("Table.b", "w");
			for (unsigned short i = 0; i < 256; i++)
			{
				fprintf(f2, "%d\n", symb[i].prob);
			}

			f3 = fopen("Codes.b", "w");
			for (unsigned short i = 0; i < 256; i++)
			{
				for (vector<unsigned char>::iterator j = symb[i].code.begin(); j != symb[i].code.end(); j++)
				{
					fprintf(f3, "%d", *j);
				}
				fprintf(f3, "\n");
			}

			f4 = fopen("TableChar.b", "w");
			for (unsigned short i = 0; i < 256; i++)
			{
				fprintf(f4, "%d\n", symb[i].ch);
			}

			fclose(f1);
			fclose(f2);
			fclose(f3);
			fclose(f4);
		}
	}

	printf("\n\nTime : %f sec\n\a\a\a", (double)clock()/CLOCKS_PER_SEC);
	printf("\nProgram has finished its job.\nPress any key to exit.\n");
	getchar();
	return 0;
}
