<?
require_once ("common.php");
check_user ();
html_start ("Main Menu");
?>
<ul>
<li><a href="newtemplate.php">Create new playlist template</a></li>
<li><a href="addevents.php">Add events to an existing template</a></li>
</ul>
<?
html_end ();
?>
