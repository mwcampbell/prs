<?
require_once ("common.php");
check_user ();
db_connect ();
$date_template = "%a %d-%b-%y %T";

if (!$source_start_date || !$source_end_date) {
	html_start ("Copy  Schedule");
?>
<form name = "copy_schedule" action="copyschedule.php" method="post">
<div>
<label for="source_start_date">Enter source start date:</label>
<input type = text name="source_start_date" id="source_start_date">
</div>
<div>
<label for="source_end_date">Enter source end date:</label>
<input type = text name="source_end_date" id="source_end_date">
</div>
<div>
<label for="dest_start_date">Enter destination start date:</label>
<input type = text name="dest_start_date" id="dest_start_date">
</div>
<div>
<label for="dest_end_date">Enter destination end date:</label>
<input type = text name="dest_end_date" id="dest_end_date">
</div>
<input type="submit" value="Copy Schedule">
</form>
<?
	html_end ();
	exit ();
}
$source_start_date = strtotime ($source_start_date);
$source_end_date = strtotime ($source_end_date);
$dest_start_date = strtotime ($dest_start_date);
$dest_end_date = strtotime ($dest_end_date);
if ($source_start_date == -1 || $source_end_date == -1 ||
	$dest_start_date == -1 || $dest_end_date == -1) {
	html_error ("Dates out of range.");
	exit ();
}
$dest_cur_time = $dest_start_date;
while ($dest_cur_time < $dest_end_date) {
	$source_cur_time = $source_start_date;
	while ($source_cur_time < $source_end_date) {
		$daylight = date ("I", $source_cur_time);
		$query = "select schedule.template_id, template_name, repeat_events,
          handle_overlap, artist_exclude, recording_exclude, start_time,
          length, repetition, fallback_id,
          end_prefade from playlist_template, schedule
          where playlist_template.template_id = schedule.template_id and
          ((repetition = 0 and start_time <= $source_cur_time and start_time+length > $source_cur_time) or
          (repetition != 0 and start_time <= $source_cur_time and mod($source_cur_time-start_time-(daylight*3600)+$daylight, repetition) < length))
          order by time_slot_id desc";
		$res = db_query ($query);
		$row = mysql_fetch_assoc ($res);
		$time_slot_id = $row["time_slot_id"];
		$start_time = $row["start_time"];
		$length = $row["length"];
		$repetition = $row["repetition"];
		$template_id = $row["template_id"];
		$fallback_id = $row["fallback_id"];
		$end_prefade = $row["end_prefade"];
		$end_time = $start_time+$length;
		$start_time = $dest_cur_time;
		$dest_cur_time = $dest_cur_time+$length;
		$query = "insert into schedule (start_time, length, repetition,
        daylight, template_id, fallback_id, end_prefade) values
        ($start_time, $length, $repetition,
        $daylight, $template_id, $fallback_id, $end_prefade)";
		db_query ($query);
		mysql_free_result ($res);
		$source_cur_time = $end_time;
	}
}
?>
<a href = "schedule.php">Back to Schedule Administration page</a>
<?
html_end ();
exit ();
?> 