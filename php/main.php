<?
require_once ("common.php");
check_user ();
html_start ("Main Menu");
?>
<ul>
<li><a href = "template.php">Manage Playlist Templates</a></li>
<li><a href = "schedule.php">Manage schedule</a></li>
<li><a href = "updatedb.php">Update Database</a></li>
</ul>
<?
html_end ();
?>
