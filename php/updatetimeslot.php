<?
require_once ("common.php");
check_user ();
db_connect ();

if ($_POST["action"] == 1) { /* Delete time slot */
	$query = "delete from schedule where time_slot_id = " . $_POST["time_slot_id"];
}
else if ($_POST["action"] == 2) { /* Modify time slot */
	$start_time = strtotime ($_POST["start_time"]);
	$end_time = strtotime ($_POST["end_time"]);
	if ($start_time == -1 || $end_time == -1) {
		html_error ("Error-- either start or end time is out of range");
		exit ();
	}
	$length = $end_time-$start_time;
	$query = "update schedule set start_time = $start_time, length = $length,
		template_id = " . $_POST["template_id"] . " where
		time_slot_id = " . $_POST["time_slot_id"];
}
if ($query)
        db_query ($query);
redirect ("viewschedule.php");
?>
