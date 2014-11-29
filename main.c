#include <stdio.h>
#include <io.h>
#include <math.h>
#include <string.h>
#include <system.h>
#include <altera_avalon_pio_regs.h>
#include <altera_avalon_timer_regs.h>
#include "basic_io.h"
#include "open_I2C.h"
#include "wm8731.h"
#include "LCD.h"
#include "SD_Card.h"
#include "fat.h"
#include <stdbool.h>
#include "inttypes.h"

static void button_ISR(void* context, alt_u32 id);
static void init_button();
static void playnormal(data_file df);
static void playdouble(data_file df);
static void playhalf(data_file df);
static void playreverse(data_file df);
static void playdelay(data_file df);
static int get_Playmode();

static int num_files;
static int current_file = 0;
volatile data_file df[20];
volatile BYTE buffer[512];
volatile int isplaying = 0;
static int playmode =0;

static int samplesPerSecond = 44100;

int main()
{
	// Initialising stuff
	printf("Initialising Player\n");
	printf("Initialising SD Card, %d\n",SD_card_init());
	printf("Initialising fat, %d\n",init_mbr());
	printf("Initialising bs, %d\n",init_bs());

	init_button();

	int filenum = -1;	
	while(filenum < (int)file_number){
		search_for_filetype( "WAV", &df[filenum], 0, 1);
		filenum++;
		//printf("file number: %d",file_number);
	}
	printf("file count: %d\n",filenum);
	num_files = filenum;
	int mode = -2;
	while(1){
		mode = get_Playmode();
		if(mode != playmode)
		{
			playmode = mode;
			LCD_Display((df[current_file].Name), playmode);
		}
		if(isplaying){
			switch(playmode){
			case 0:
				playnormal(df[current_file]);
				break;
			case 1:
				playdouble(df[current_file]);
				break;
			case 2:
				playhalf(df[current_file]);
				break;
			case 3:
				playdelay(df[current_file]);
				break;
			case 4:
				playreverse(df[current_file]);
				break;
			default:

				break;
			}
			isplaying = 0;
		}
	}
}
static int get_Playmode()
{
	int mode;
	switch(IORD(SWITCH_PIO_BASE, 0)){
	case 1:
		mode = 0;
		break;
	case 2:
		mode = 1;
		break;
	case 4:
		mode = 2;
		break;
	case 8:
		mode = 3;
		break;
	case 16:
		mode = 4;
		break;
	default:
		mode = -1;
		break;
	}
	return mode;
}

static void playnormal(data_file df){
	LCD_File_Buffering(df.Name);
	int cc[4000];
	int length = 1 + ceil(df.FileSize/(BPB_BytsPerSec*BPB_SecPerClus));
	build_cluster_chain(cc,length,&df);
	//printf("FileSize: %d\n",length);
	int j =0;

	LCD_Display((df.Name), playmode);
	while(get_rel_sector(&df,buffer,cc,j)== 0){
		//printf("sector number: %d\n",j);
		UINT16 tmp; //Create a 16-bit variable to pass to the FIFO
		int i = 0;
		while(i < BPB_BytsPerSec){
			if(!isplaying){
				return;
			}
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			//Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			tmp = (buffer[ i + 1 ] << 8 ) | ( buffer[ i ] );
			//Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC
			IOWR(AUDIO_0_BASE, 0, tmp );

			i+=2;
		}
		j++;
	}
}

static void playdouble(data_file df){
	LCD_File_Buffering(df.Name);
	int cc[4000];
	int length = 1 + ceil(df.FileSize/(BPB_BytsPerSec*BPB_SecPerClus));
	build_cluster_chain(cc,length,&df);
	//printf("FileSize: %d\n",length);
	int j =0;
	LCD_Display((df.Name), playmode);
	while(get_rel_sector(&df,buffer,cc,j)== 0){
		//printf("sector number: %d\n",j);
		UINT16 tmp; //Create a 16-bit variable to pass to the FIFO
		int i = 0;
		while(i < BPB_BytsPerSec){
			if(!isplaying){
				return;
			}
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			//Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			tmp = (buffer[ i + 1 ] << 8 ) | ( buffer[ i ] );

			//Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC
			IOWR(AUDIO_0_BASE, 0, tmp );
			i+=4;
		}
		j++;
	}
}
static void playhalf(data_file df){
	LCD_File_Buffering(df.Name);
	int cc[4000];
	int length = 1 + ceil(df.FileSize/(BPB_BytsPerSec*BPB_SecPerClus));
	build_cluster_chain(cc,length,&df);

	int j =0;
	LCD_Display((df.Name), playmode);
	while(get_rel_sector(&df,buffer,cc,j)== 0){
		UINT16 tmp; //Create a 16-bit variable to pass to the FIFO
		int i = 0;
		while(i < BPB_BytsPerSec){
			if(!isplaying){
				return;
			}
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			//Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			tmp = (buffer[ i + 1 ] << 8 ) | ( buffer[ i ] );

			//Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC
			IOWR(AUDIO_0_BASE, 0, tmp );
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			IOWR(AUDIO_0_BASE, 0, tmp );
			i+=2;
		}
		j++;
	}
}
static void playdelay(data_file df){
	LCD_File_Buffering(df.Name);
	int cc[4000];
	int length = 1 + ceil(df.FileSize/(BPB_BytsPerSec*BPB_SecPerClus));
	build_cluster_chain(cc,length,&df);
	int16_t channel_buf[samplesPerSecond];

	int channelBuf_ptr = 0;
	while(channelBuf_ptr < samplesPerSecond){
		channel_buf[channelBuf_ptr] = 0;
		channelBuf_ptr++;
	}
	channelBuf_ptr = 0;

	int j = 0;
	int isRightChannel = 0;
	LCD_Display((df.Name), playmode);

	while(get_rel_sector(&df,buffer,cc,j)== 0){
		UINT16 tmp; //Create a 16-bit variable to pass to the FIFO
		int i = 0;
		while(i < BPB_BytsPerSec){
			if(!isplaying){
				return;
			}
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			//Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			tmp = (buffer[ i + 1 ] << 8 ) | ( buffer[ i ] );

			//Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC

			if(isRightChannel){
				IOWR(AUDIO_0_BASE, 0, channel_buf[channelBuf_ptr]);
				channel_buf[channelBuf_ptr] = tmp;
				isRightChannel = 0;
				channelBuf_ptr++;
				if(channelBuf_ptr >= samplesPerSecond){
					channelBuf_ptr = 0;
				}
			}
			else{
			IOWR(AUDIO_0_BASE, 0, tmp );
			isRightChannel = 1;
			}
			i+=2;
		}
		j++;
	}
	int k = 0;
	while(k < samplesPerSecond){
		if(isRightChannel){
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			IOWR(AUDIO_0_BASE, 0, channel_buf[channelBuf_ptr]);
			isRightChannel = 0;
			k++;
			channelBuf_ptr++;
			if(channelBuf_ptr == samplesPerSecond){
				channelBuf_ptr = 0;
			}
		}else{
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			IOWR(AUDIO_0_BASE, 0, 0);
			isRightChannel = 1;
		}
	}
}

static void playreverse(data_file df){
	LCD_File_Buffering(df.Name);
	int cc[4000];
	int length = 1 + ceil(df.FileSize/(BPB_BytsPerSec*BPB_SecPerClus));
	int sectorNumber = 0;
	int lastSectorSize;
	int isLastSector = 1;
	build_cluster_chain(cc,length,&df);
	//printf("FileSize: %d\n",length);

	//while(get_rel_sector(&df,buffer,cc,sectorNumber)== 0){
	//	sectorNumber++;
	//}
	sectorNumber = ceil(df.FileSize / BPB_BytsPerSec);
	lastSectorSize = get_rel_sector(&df,buffer,cc,sectorNumber);
	int j = sectorNumber;
	LCD_Display((df.Name), playmode);

	while(j >= 0){
		//printf("sector number: %d\n",j);
		get_rel_sector(&df,buffer,cc,j);
		UINT16 tmp; //Create a 16-bit variable to pass to the FIFO
		int i = 512;
		if(isLastSector){
			i = lastSectorSize;
			isLastSector = 0;
		}
		if(!isplaying){
			return;
		}
		while(i >= 0){
			if(!isplaying){
				return;
			}
			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			//Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			tmp = (buffer[ i - 3 ] << 8) | ( buffer[ i - 2 ]);

			//Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC
			IOWR(AUDIO_0_BASE, 0, tmp );

			while(IORD( AUD_FULL_BASE, 0 ) ) {} //wait until the FIFO is not full
			//Package 2 8-bit bytes from the
			//sector buffer array into the
			//single 16-bit variable tmp
			tmp = (buffer[ i - 1 ] << 8) | ( buffer[ i ]);

			//Write the 16-bit variable tmp to the FIFO where it
			//will be processed by the audio CODEC
			IOWR(AUDIO_0_BASE, 0, tmp );
			i-=4;
		}
		j--;
	}
}

static void init_button()
{
	/* set up the interrupt vector */
	alt_irq_register( BUTTON_PIO_IRQ, (void*)0, button_ISR);
	/* reset the edge capture register by writing to it (any value will do) */
	IOWR(BUTTON_PIO_BASE, 3, 0x0);
	/* enable interrupts for all four buttons*/
	IOWR(BUTTON_PIO_BASE, 2, 0xf);

	printf("Button Initialized\n");
}

static void button_ISR(void* context, alt_u32 id)
{
	alt_u8 buttons;

	/* get value from edge capture register and mask off all bits except
	the 4 least significant */
	buttons = IORD(BUTTON_PIO_BASE, 3) & 0xf;


	switch(buttons){
		case 1:
			//printf("button one pressed\n");
			isplaying = 0;
			break;
		case 2:
			//printf("button2current file: %d\n",current_file);
			isplaying = 1;
			LCD_Display((df[current_file].Name), playmode);

			break;
		case 4 :
			if(!isplaying)
			{
				current_file++;
				if(current_file == num_files){
					current_file = 0;
				}
				LCD_Display((df[current_file].Name), playmode);
				//printf("button3current file: %d\n",current_file);
			}

			break;
		case 8:
			if(!isplaying){
				current_file--;
				if(current_file == -1){
					current_file = num_files-1;
				}
				LCD_Display((df[current_file].Name), playmode);
				//printf("button4current file: %d\n",current_file);
				break;
			}
		default:
			printf("button: %d\n",buttons);
			break;
	}
    /* reset edge capture register to clear interrupt */
	IOWR(BUTTON_PIO_BASE, 3, 0x0);
}
