#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileinfo.h"
#include "mp3decoder.h"
#include "mp3fileinfo.h"
#include "mp3tech.h"



/* Genre list taken from mp3info.h in the mp3info package */
#define MAX_GENRE 147
static char *genres[MAX_GENRE + 2] = {
  "Blues","Classic Rock","Country","Dance","Disco","Funk","Grunge",
  "Hip-Hop","Jazz","Metal","New Age","Oldies","Other","Pop","R&B",
  "Rap","Reggae","Rock","Techno","Industrial","Alternative","Ska",
  "Death Metal","Pranks","Soundtrack","Euro-Techno","Ambient",
  "Trip-Hop","Vocal","Jazz+Funk","Fusion","Trance","Classical",
  "Instrumental","Acid","House","Game","Sound Clip","Gospel","Noise",
  "Alt. Rock","Bass","Soul","Punk","Space","Meditative",
  "Instrumental Pop","Instrumental Rock","Ethnic","Gothic",
  "Darkwave","Techno-Industrial","Electronic","Pop-Folk","Eurodance",
  "Dream","Southern Rock","Comedy","Cult","Gangsta Rap","Top 40",
  "Christian Rap","Pop/Funk","Jungle","Native American","Cabaret",
  "New Wave","Psychedelic","Rave","Showtunes","Trailer","Lo-Fi",
  "Tribal","Acid Punk","Acid Jazz","Polka","Retro","Musical",
  "Rock & Roll","Hard Rock","Folk","Folk/Rock","National Folk",
  "Swing","Fast-Fusion","Bebob","Latin","Revival","Celtic",
  "Bluegrass","Avantgarde","Gothic Rock","Progressive Rock",
  "Psychedelic Rock","Symphonic Rock","Slow Rock","Big Band",
  "Chorus","Easy Listening","Acoustic","Humour","Speech","Chanson",
  "Opera","Chamber Music","Sonata","Symphony","Booty Bass","Primus",
  "Porn Groove","Satire","Slow Jam","Club","Tango","Samba",
  "Folklore","Ballad","Power Ballad","Rhythmic Soul","Freestyle",
  "Duet","Punk Rock","Drum Solo","A Cappella","Euro-House",
  "Dance Hall","Goa","Drum & Bass","Club-House","Hardcore","Terror",
  "Indie","BritPop","Negerpunk","Polsk Punk","Beat",
  "Christian Gangsta Rap","Heavy Metal","Black Metal","Crossover",
  "Contemporary Christian","Christian Rock","Merengue","Salsa",
  "Thrash Metal","Anime","JPop","Synthpop",""
};



static double
get_mp3_audio_in (FileInfo *info, int threshhold)
{
  MP3Decoder *d = NULL;
  short *buffer = NULL, *end_buffer = NULL;
  int buffer_size, samples_read;
  short *ptr;
  double audio_in = 0.0;
  int done = 0;
  
  d = mp3_decoder_new (info->path, 0);
  buffer_size = info->rate * info->channels;
  buffer = (short *) malloc (buffer_size * sizeof (short));

  while (!done)
    {
      samples_read = mp3_decoder_get_data (d, buffer, buffer_size);
      end_buffer = buffer + samples_read;
      ptr = buffer;
      while (ptr < end_buffer)
	{
	  if (*ptr > threshhold || *ptr < -threshhold)
	    {
	      audio_in += (double) (ptr - buffer) / buffer_size;
	      done = 1;
	      break;
	    }
	  ptr++;
	}

      if (!done)
	audio_in += (double) samples_read / buffer_size;
    }
  mp3_decoder_destroy (d);
  free (buffer);
  return audio_in;
}



static double
get_mp3_audio_out (FileInfo *info, mp3info *mp3, int threshhold)
{
  MP3Decoder *d = NULL;
  short *buffer = NULL, *end_buffer = NULL;
  int buffer_size, samples_read;
  short *ptr;
  double audio_out;
  double seek_time;
  double fps = (double) mp3->frames / mp3->seconds;
  int skip_frames = 0;
  
  buffer_size = info->rate * info->channels * 12;
  buffer = (short *) malloc (buffer_size * sizeof (short));

  seek_time = info->length - 10;
  if (seek_time < 0)
    seek_time = 0;
  skip_frames = (int) (seek_time * fps);
  seek_time = skip_frames / fps;
  d = mp3_decoder_new (info->path, skip_frames);

  samples_read = mp3_decoder_get_data (d, buffer, buffer_size);
  end_buffer = buffer + samples_read;
  ptr = end_buffer;
  while (ptr > buffer)
    {
      if (*ptr > threshhold || *ptr < -threshhold)
	break;
      ptr--;
    }
  audio_out =
    seek_time + (double) (ptr - buffer) / (info->rate * info->channels);
  mp3_decoder_destroy (d);
  free (buffer);
  return audio_out;
}



FileInfo *
get_mp3_file_info (char *path, unsigned short threshhold)
{
  mp3info mp3;
  FileInfo *info;

  memset (&mp3, 0, sizeof (mp3));
  mp3.file = fopen (path, "rb");
  if (!mp3.file)
    return NULL;
  mp3.filename = path;
  get_mp3_info (&mp3, SCAN_FULL, 1);
  fclose (mp3.file);
  mp3.file = NULL;
  mp3.filename = NULL;

  /* Allocate the FileInfo structure */

  info = (FileInfo *) malloc(sizeof(FileInfo));
  if (!info)
    return NULL;
  
  /* Fill in structure */

  info->name = NULL;
  info->path = strdup (path);
  info->artist = NULL;
  info->genre = NULL;
  info->date = NULL;
  info->album = NULL;
  info->track_number = NULL;
  info->rate = header_frequency (&mp3.header);
  info->channels = (mp3.header.mode == 3) ? 1 : 2;
  info->length = mp3.seconds;
  info->audio_in = -1.0;
  info->audio_out = -1.0;

  if (mp3.id3_isvalid)
    {
      info->name = strdup (mp3.id3.title);
      info->artist = strdup (mp3.id3.artist);
      info->album = strdup (mp3.id3.album);
      info->date = strdup (mp3.id3.year);

      if (mp3.id3.genre[0] <= MAX_GENRE)
	info->genre = strdup (genres[mp3.id3.genre[0]]);

      if (mp3.id3.track[0] > 0)
	{
	  info->track_number = malloc (4);
	  sprintf (info->track_number, "%u", mp3.id3.track[0]);
	}
    }
  
  if (threshhold > 0)
    {
      info->audio_in = get_mp3_audio_in (info, threshhold);
      info->audio_out = get_mp3_audio_out (info, &mp3, threshhold);
    }
  
  return info;
}
