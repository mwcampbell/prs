<?
require_once ("common.php");
check_user ();
html_start ("Manage Playlist Templates");
?>
<ul>
<li><a href="newtemplate.php">Create New Playlist Template</a></li>
<li><a href = "edittemplate.php">Edit an Existing Playlist Template</a></li>
<li><a href = "main.php">Back to Main Menu</a></li>
</ul>
<?
html_end ();
?>
