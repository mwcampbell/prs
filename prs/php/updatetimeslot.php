<?
require_once ("common.php");
check_user ();
db_connect ();
if ($action == 1) { /* Delete time slot */
	$query = "delete from schedule where time_slot_id = $time_slot_id";
}
else if ($action == 2) { /* Modify time slot */
	$start_time = strtotime ($start_time);
	$end_time = strtotime ($end_time);
	if ($start_time == -1 || $end_time == -1) {
		html_error ("Error-- either start or end time is out of range");
		exit ();
	}
	$length = $end_time-$start_time;
	$query = "update schedule set start_time = $start_time, length = $length, template_id = $template_id where time_slot_id = $time_slot_id";
}
if ($query)
        db_query ($query);
redirect ("viewschedule.php");
?>