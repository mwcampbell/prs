<?
require_once ("common.php");
check_user ();
db_connect ();

/* Ensure that $template_events_repeat contains something */

if (!$_POST["template_events_repeat"])
	$template_events_repeat = "0";
else $template_events_repeat = $_POST["template_events_repeat"];

/* Connect to the prs database */

if ($_POST["new_template"])
{

	list ($artist_exclude_hours, $artist_exclude_minutes) =
		explode (":", $_POST["template_artist_exclude"], 2);
	$artist_exclude = $artist_exclude_hours*3600+$artist_exclude_minutes*60;
	list ($recording_exclude_hours, $recording_exclude_minutes) =
		explode (":", $_POST["template_recording_exclude"], 2);
	$recording_exclude = $recording_exclude_hours*3600+$recording_exclude_minutes*60;

	$query = "insert into playlist_template (template_name, repeat_events, handle_overlap, artist_exclude, recording_exclude) values ('" . addslashes ($_POST["template_name"]) . "', $template_events_repeat, " . $_POST["template_handle_overlap"] . ", $artist_exclude, $recording_exclude)";
	db_query ($query);
	$template_id = mysql_insert_id ();
} else if ($_SERVER["REQUEST_METHOD"] == "GET")
	$template_id = $_GET["template_id"];
else $template_id = $_POST["template_id"];

// At this point, $template_id has the ID of the template, either from MySQL,
// from a form, or from createvent.php via a redirect (which uses "get".

redirect ("addevents.php?template_id=" . $template_id);
?>

