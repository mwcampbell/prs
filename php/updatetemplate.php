<?
require_once ("common.php");
check_user ();
db_connect ();

// The following is used to convert a string "hh:mm" to seconds.
// Return -1 on failure.
function timenum ($string) {
	if ($string == "")
		return 0;
	if ((strlen ($string) <> 5) or (strpos ($string, ":") <> 2))
		return -1;

	list ($hours, $minutes) = explode (":", $string, 2);
	if (!is_numeric ($hours) or !is_numeric ($minutes))
		return -1;
	if ($hours < 0)
		return -1;
	if (($minutes > 59) or ($minutes < 0))
		return -1;

	$seconds = intval ($hours*3600) + intval ($minutes*60);
	return $seconds;
} // end timenum()


if ($_POST["new_template"])
	$template_id = -1; // This wil be set properly later
else $template_id = $_POST["template_id"];

$template_name = $_POST["template_name"];
if ($template_name == "")
	html_error ("You must enter a template name.");

// Check for duplicate template names
$duplicate = false;
$res = db_query ("select template_name from playlist_template");
while ($row = mysql_fetch_assoc ($res) and !$duplicate)
	if ((strcasecmp ($template_name, $row["template_name"]) == 0) and
	($template_id <> $row["template_id"]))
		$duplicate = true;
	mysql_free_result ($res);
if ($duplicate)
	html_error ("Sorry, but a template with the name $template_name already exists.");

if (!$_POST["template_events_repeat"])
	$template_events_repeat = "0";
else $template_events_repeat = "1";

$artist_exclude = timenum ($_POST["template_artist_exclude"]);
if ($artist_exclude == -1)
	$bad_artist_exclude = true;

$recording_exclude = timenum ($_POST["template_recording_exclude"]);
if ($recording_exclude == -1)
	$bad_recording_exclude = true;

if ($bad_artist_exclude or $bad_recording_exclude) {
	$error_string = "Sorry, but you have specified a bad value for ";
	if ($bad_artist_exclude)
		$error_string .= "the minimum time between recordings by the same artist";
	if ($bad_artist_exclude and $bad_recording_exclude)
		$error_string .= " and ";
	if ($bad_recording_exclude)
		$error_string .= "the minimum time between repetitions of a recording";
	$error_string .= ".  The correct format is hh:mm where h=hours ";
	$error_string .= "and m=minutes.  The highest possible value is 99:59.";
	html_error ($error_string);
} // end if bad exclude values


if ($_POST["new_template"])
{

	$query = "insert into playlist_template (template_name, repeat_events, handle_overlap, artist_exclude, recording_exclude) values ('" . addslashes ($template_name) . "', $template_events_repeat, " . $_POST["template_handle_overlap"] . ", $artist_exclude, $recording_exclude)";
	db_query ($query);
	$template_id = mysql_insert_id ();

	if (!$_POST["load_events"])
		redirect ("addevents.php?template_id=" . $template_id);

// Copy events from the original template.
	$query = "select * from playlist_event where template_id = " . $_POST["original_template_id"] . " order by event_number";
	$res = db_query ($query);
	while ($row = mysql_fetch_assoc ($res)) {
		$query = "insert into playlist_event
		(template_id, event_number,
		event_name, event_type,
		event_channel_name, event_level,
		event_anchor_event_number,
		event_anchor_position,
		event_offset,
		detail1, detail2,
		detail3, detail4,
		detail5) values (" .
		$template_id . ', ' . $row["event_number"] . ', ' .
		'"' . $row["event_name"] . '", "' . $row["event_type"] . '", ' .
		'"' . $row["event_channel_name"] . '", ' . $row["event_level"] . ', ' .
		$row["event_anchor_event_number"] . ', ' .
		$row["event_anchor_position"] . ', ' . $row["event_offset"] . ', ' .
		'"' . $row["detail1"] . '", "' . $row["detail2"] . '", ' .
		'"' . $row["detail3"] . '", "' . $row["detail4"] . '", ' .
		'"' . $row["detail5"] . '")';
		db_query ($query);
	} // end traversal of template events
	mysql_free_result ($res);
} // end if new template

else {
// Update existing template
	$query = "update playlist_template set " .
	"template_name='" . addslashes ($template_name) . "', " .
	"repeat_events=$template_events_repeat, " .
	"handle_overlap='" . $_POST["template_handle_overlap"] . "', " .
	"artist_exclude=$artist_exclude, " .
	"recording_exclude=$recording_exclude " .
	"where template_id=$template_id";
	db_query ($query);
} // end if editing a template.

// Now we have either an updated template or a new template with events.
redirect ("edittemplate.php?template_id=$template_id");
?>
