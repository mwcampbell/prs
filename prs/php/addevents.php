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

	/* parse times */

	$start_time = strtotime ($template_start_time);
	if ($start_time == -1)
		html_error ("Start time is not in a valid format.");
	if ($start_time < time ())
		html_error ("Start time is in the past.");
	$end_time = strtotime ($template_end_time);
	if ($end_time == -1)
		html_error ("End time is not in a valid format.");
	if ($end_time < time ())
		html_error ("End time is in the past.");
	if ($end_time < $start_time)
		html_error ("End time is less than start time.");
	list ($artist_exclude_hours, $artist_exclude_minutes) =
		explode (":", $template_artist_exclude, 2);
	$artist_exclude = $artist_exclude_hours*3600+$artist_exclude_minutes*60;
	list ($recording_exclude_hours, $recording_exclude_minutes) =
		explode (":", $template_recording_exclude, 2);
	$recording_exclude = $recording_exclude_hours*3600+$recording_exclude_minutes*60;

	$query = "insert into playlist_template (template_name, start_time, end_time, repeat_events, artist_exclude, recording_exclude) values ('" . addslashes ($template_name) . "', $start_time, $end_time, $template_events_repeat, $artist_exclude, $recording_exclude)";
	db_query ($query);
	$template_it = mysql_insert_id ();
}
else if (!$template_id)
{
	$res = db_query("select template_id, template_name from playlist_template");
	html_start ("Select Playlist Template");
?>
<form name="select_template" action="addevents.php" method="post">
<div>
<select name="template_id">
<?
	while ($row = mysql_fetch_assoc ($res))
	{
		$id = $row["template_id"];
		$name = $row["template_name"];
		echo ("<option value=\"$id\">$name</option>\n");
	}
	mysql_free_result ($res);
?>
</select><br>
<input type="submit" value="Add Events">
</div>
</form>
<?
	html_end ();
	exit ();
}

html_start ("Add Playlist Event");
?>
<ul>
<li><a href="randomevent.php?template_id=<? echo $template_id ?>&amp;event_type=random">Add random event</a></li>
<li><a href="randomevent.php?template_id=<? echo $template_id ?>&amp;event_type=simple_random">Add simple random event</a></li>
</ul>
<?
html_end ();
?>
