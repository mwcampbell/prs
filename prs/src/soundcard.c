#include "soundcard.h"




static int soundcard_fd = -1;
static int soundcard_rate;
static int soundcard_channels = -1;
static int soundcard_duplex = -1;

int
soundcard_get_fd (void)
{
	return soundcard_fd;
}


void
soundcard_set_fd (int fd)
{
	soundcard_fd = fd;
}


int
soundcard_get_rate (void)
{
	return soundcard_rate;
}


void
soundcard_set_rate (int rate)
{
	soundcard_rate = rate;
}


int
soundcard_get_channels (void)
{
	return soundcard_channels;
}


void
soundcard_set_channels (int channels)
{
	soundcard_channels = channels;
}


int
soundcard_get_duplex (void)
{
	return soundcard_duplex;
}


void
soundcard_set_duplex (int duplex)
{
	soundcard_duplex = duplex;
}
