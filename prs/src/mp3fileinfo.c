#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <id3.h>
#include "fileinfo.h"
#include "mp3decoder.h"
#include "mp3fileinfo.h"
#include "mp3header.h"



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



static int
mp3_scan (FileInfo *info)
{
  int frames = 0, samples = 0;
  FILE *fp = fopen (info->path, "rb");
  mp3_header_t mh;
  int sample_rate = -1, channels = -1;

  while (mp3_header_read (fp, &mh))
    {
      frames++;
      if (sample_rate == -1)
	sample_rate = mh.samplerate;
      if (channels == -1)
	channels = (mh.mode == MPEG_MODE_MONO) ? 1 : 2;
      samples += mh.samples;
      fseek (fp, mh.framesize, SEEK_CUR);
    }

  fclose (fp);
  info->length = (double) samples / sample_rate;
  info->rate = sample_rate;
  info->channels = channels;
  return frames;
}

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
get_mp3_audio_out (FileInfo *info, int frames, int threshhold)
{
  MP3Decoder *d = NULL;
  short *buffer = NULL, *end_buffer = NULL;
  int buffer_size, samples_read;
  short *ptr;
  double audio_out;
  double seek_time;
  double fps = (double) frames / info->length;
  int skip_frames = 0;
  
  buffer_size = info->rate * info->channels * 20;
  buffer = (short *) malloc (buffer_size * sizeof (short));

  seek_time = info->length - 10;
  if (seek_time < 0)
    seek_time = 0;
  skip_frames = (int) (seek_time * fps);
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
    info->length - (double) (end_buffer - ptr) / (info->rate * info->channels);
  mp3_decoder_destroy (d);
  free (buffer);
  return audio_out;
}



FileInfo *
get_mp3_file_info (char *path, unsigned short in_threshhold,
		   unsigned short out_threshhold)
{
  FileInfo *info;
  ID3Tag *tag;
  ID3Frame *frame = NULL;
  ID3Field *field = NULL;
  int frames;

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
  frames = mp3_scan (info);
  info->audio_in = -1.0;
  info->audio_out = -1.0;

  /* Get ID3 info. */
  tag = ID3Tag_New ();
  ID3Tag_Link (tag, path);

  if ((frame = ID3Tag_FindFrameWithID (tag, ID3FID_TITLE)) != NULL &&
      (field = ID3Frame_GetField (frame, ID3FN_TEXT)) != NULL)
    {
      char title[256];
      ID3Field_GetASCII (field, title, 256);
      info->name = strdup (title);
    }
  
  if ((frame = ID3Tag_FindFrameWithID (tag, ID3FID_LEADARTIST)) != NULL &&
      (field = ID3Frame_GetField (frame, ID3FN_TEXT)) != NULL)
    {
      char artist[256];
      ID3Field_GetASCII (field, artist, 256);
      info->artist = strdup (artist);
    }
  
  if ((frame = ID3Tag_FindFrameWithID (tag, ID3FID_ALBUM)) != NULL &&
      (field = ID3Frame_GetField (frame, ID3FN_TEXT)) != NULL)
    {
      char album[256];
      ID3Field_GetASCII (field, album, 256);
      info->album = strdup (album);
    }
  
  if ((frame = ID3Tag_FindFrameWithID (tag, ID3FID_YEAR)) != NULL &&
      (field = ID3Frame_GetField (frame, ID3FN_TEXT)) != NULL)
    {
      char date[256];
      ID3Field_GetASCII (field, date, 256);
      info->date = strdup (date);
    }
  
  if ((frame = ID3Tag_FindFrameWithID (tag, ID3FID_CONTENTTYPE)) != NULL &&
      (field = ID3Frame_GetField (frame, ID3FN_TEXT)) != NULL)
    {
      char genre[256];
      ID3Field_GetASCII (field, genre, 256);
      info->genre = strdup (genre);
    }
  
  ID3Tag_Delete (tag);
  
  if (in_threshhold > 0)
    info->audio_in = get_mp3_audio_in (info, in_threshhold);
  if (out_threshhold > 0)
    info->audio_out = get_mp3_audio_out (info, frames, out_threshhold);
  
  return info;
}
