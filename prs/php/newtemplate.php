<?
require_once ("common.php");
check_user ();
html_start ("Create Playlist Template");
?>
<form name="newtemplate" action="addevents.php" method="post">
<div>
<input type="hidden" name="new_template" value="yes">
<label for="template_name">Name:</label>
<input type="text" name="template_name" id="template_name">
<br>
<label for="template_start_time">Start time:</label>
<input type="text" name="template_start_time" id="template_start_time">
<br>
<label for="template_end_time">End time:</label>
<input type="text" name="template_end_time" id="template_end_time">
<br>
<input type="checkbox" name="template_events_repeat" value="1" id="template_events_repeat">
<label for="template_events_repeat">Repeat events in this template until the template's end time has been reached.</label>
<br>
<label for="template_artist_exclude">Minimum time between recordings from the same artist (HH:MM):</label>
<input type="text" name="template_artist_exclude" id="template_artist_exclude">
<br>
<label for="template_recording_exclude">Minimum time between repetitions of a recording (HH:MM):</label>
<input type="text" name="template_recording_exclude" id="template_recording_exclude">
<br>
<input type="submit" value="Create Template">
</div>
</form>
<?
html_end ();
?>
