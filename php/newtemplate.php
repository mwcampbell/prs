<?
require_once ("common.php");
check_user ();
html_start ("Create Playlist Template");
?>
<form name="newtemplate" action="updatetemplate.php" method="post">
<div>
<input type="hidden" name="new_template" value="yes">
<label for="template_name">Name:</label>
<input type="text" name="template_name" id="template_name">
<br>
<input type="checkbox" name="template_events_repeat" value="1" id="template_events_repeat">
<label for="template_events_repeat">Repeat events in this template until the template's end time has been reached.</label>
<br>
<label for="template_handle_overlap">What to do when a recording won't fit:</label>
<select id="template_handle_overlap" name="template_handle_overlap" value="1">
<option value="1">Discard</option>
<option value="2">Switch to fallback template</option>
<option value="3">Fade audio at end of template</option>
<option value="4">Ignore</option>
</select>
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
