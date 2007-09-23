<?
require_once ("common.php");
check_user ();
db_connect ();

/* Ensure that $template_events_repeat contains something */

if (!$template_events_repeat)
	$template_events_repeat = "0";

/* Connect to the prs database */

if ($new_template)
{

	list ($artist_exclude_hours, $artist_exclude_minutes) =
		explode (":", $template_artist_exclude, 2);
	$artist_exclude = $artist_exclude_hours*3600+$artist_exclude_minutes*60;
	list ($recording_exclude_hours, $recording_exclude_minutes) =
		explode (":", $template_recording_exclude, 2);
	$recording_exclude = $recording_exclude_hours*3600+$recording_exclude_minutes*60;

	$query = "insert into playlist_template (template_name, repeat_events, handle_overlap, artist_exclude, recording_exclude) values ('" . addslashes ($template_name) . "', $template_events_repeat, $template_handle_overlap, $artist_exclude, $recording_exclude)";
	db_query ($query);
	$template_id = mysql_insert_id ();
}

html_start ("Add Playlist Event");
?>
<ul>
<li><a href="randomevent.php?template_id=<? echo $template_id ?>&amp;event_number=<? echo $event_number ?>&amp;insert_event=<? echo $insert_event ?>&amp;event_type=random">Add Random Event</a></li>
<li><a href="randomevent.php?template_id=<? echo $template_id ?>&amp;event_number=<? echo $event_number ?>&amp;insert_event=<? echo $insert_event ?>&amp;event_type=simple_random">Add Simple Random Event</a></li>
<li><a href="urlevent.php?template_id=<? echo $template_id ?>&amp;event_number=<? echo $event_number ?>&amp;insert_event=<? echo $insert_event ?>">Add URL Event</a></li>
<li><a href="pathevent.php?template_id=<? echo $template_id ?>&amp;event_number=<? echo $event_number ?>&amp;insert_event=<? echo $insert_event ?>">Add Path Event</a></li>
<li><a href = "edittemplate.php?template_id=<? echo $template_id ?>">Edit this template</a></li>
<li><a href = "template.php">Back to Playlist Template Administration</a></li>
</ul>
<?
html_end ();
?>
