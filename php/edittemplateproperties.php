<?
require_once ("common.php");
check_user ();
db_connect ();

// The following is used to convert seconds to a string "hh:mm".
function timestring ($seconds) {
	$hours = 0;
	$minutes = 0;
	if ($seconds >= 3600) {
		$hours = ($seconds - ($seconds % 3600)) / 3600;
		$seconds = $seconds % 3600;
} // end if at least 1 hour

// Times should be in hole minutes since this is how they are entered.
	$minutes = $seconds / 60;
	return (sprintf ("%02d:%02d", $hours, $minutes));
} // end timestring()

// $action will be either "new", "load" or "edit".

// Request type will only be "get" if called from the Main Menu
// as "Create New Template".

if ($_SERVER["REQUEST_METHOD"] == "GET")
	$action = "new";
else {
	$action = $_POST["action"];
	$template_id = $_POST["template_id"];
} // end if post (either "edit" or "load")

if ($action == "edit" or $action == "load") {
	$res = db_query ("select * from playlist_template where template_id=" . $template_id);
	$row = mysql_fetch_assoc ($res);
	$template_name = $row["template_name"];
	$repeat_events = $row["repeat_events"];
	$handle_overlap = $row["handle_overlap"];
	$artist_exclude = $row["artist_exclude"];
	$recording_exclude = $row["recording_exclude"];
	mysql_free_result ($res);
} // end if loading a template

if ($action == "edit")
	html_start ("Edit Template Properties");
else html_start ("Create Playlist Template");

if ($action == "new" or ($action == "load" and $template_id == "-1")) {
?>

<P>
<form action="edittemplateproperties.php" method="post">
<input type=hidden name=action value="load">
<label for="template_id">Initialise template using</label>
<select id="template_id" name="template_id">
<?php
display_template_list (-1);
?>
</select><br>
<input id="load_events" name="load_events" type="checkbox">
<label for="load_events">Include events</label>
<input type=submit value="Load">
</form>
<HR>
</p>

<?php
} // end if new template
?>

<P>
<form name="edittemplateproperties" action="updatetemplate.php" method="post">
<div>

<?php
if ($action == "new" or $action == "load")
	echo '<input type="hidden" name="new_template" value="yes">' . "\n";

echo '<label for="template_name">Name:</label>' . "\n";
echo '<input type="text" name="template_name" id="template_name"';
if ($template_name)
	echo ' value="' . htmlspecialchars($template_name) . '"';
echo "><br>\n";

echo '<input type="checkbox" name="template_events_repeat" id="template_events_repeat"';
if ($repeat_events == 1)
	echo " checked";
echo ">\n";
echo '<label for="template_events_repeat">Repeat events in this template ';
echo "until the template's end time has been reached.</label><br>\n";

echo '<label for="template_handle_overlap">What to do when a recording won\'t fit:</label>' . "\n";
echo '<select id="template_handle_overlap" name="template_handle_overlap">' . "\n";
echo '<option value="1"';
if ($handle_overlap == 1)
	echo " selected";
echo ">Discard</option>\n";
echo '<option value="2"';
if ($handle_overlap == 2)
        echo " selected";
echo ">Switch to fallback template</option>\n";
echo '<option value="3"';
if ($handle_overlap == 3)
        echo " selected";
echo ">Fade audio at end of template</option>\n";
echo '<option value="4"';
if ($handle_overlap == 4)
        echo " selected";
echo ">Ignore</option>\n";
echo "</select><br>\n";

echo '<label for="template_artist_exclude">Minimum time between recordings from the same artist (HH:MM):</label>' . "\n";
echo '<input type="text" name="template_artist_exclude" id="template_artist_exclude"';
if ($artist_exclude)
	echo ' value="' . timestring ($artist_exclude) . '"';
echo "><br>\n";

echo '<label for="template_recording_exclude">Minimum time between repetitions of a recording (HH:MM):</label>' . "\n";
echo '<input type="text" name="template_recording_exclude" id="template_recording_exclude"';
if ($recording_exclude)
	echo ' value="' . timestring ($recording_exclude) . '"';
echo "><br>\n";

echo '<input type="submit" value="';
if ($action == "edit")
	echo "Update Template";
else echo "Create Template";
echo '">' . "\n</div>\n</form>\n";
html_end ();
?>
