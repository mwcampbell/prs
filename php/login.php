<?
require_once ("common.php");

parse_config_file ($_POST["station");

db_connect ();

if (!$_POST["username"] && !$_SESSION["user_username"])
	html_error ("Login error.");
else if (!$_SESSION["user_username"])
{
	$query = "select password, type from prs_user where user_name = '" . $_POST["username"] . "'";
	$res = db_query ($query);
	if ($row = mysql_fetch_assoc ($res))
	{
		$real_password = $row["password"];
		$real_type = $row["type"];
	}
	mysql_free_result ($res);
	if (!$real_password)
	{
		html_error ("there is no user by that name.");
	}
	else if ($real_password != $_POST["password"])
	{
		html_error ("Wrong password.");
	}
	else
	{
// We can assign to $_SESSION because session_start() was called in common.php
		$_SESSION["user_username"] = $_POST["username"];
		$_SESSION["user_password"] = $real_password;
		$_SESSION["user_type"] = $real_type;
		$_SESSION["station"] = $_POST["station"];
		$_SESSION["DB_HOST"] = $DB_HOST;
		$_SESSION["DB_NAME"] = $DB_NAME;
		$_SESSION["DB_USER"] = $DB_USER;
		$_SESSION["DB_PASSWORD"] = $DB_PASSWORD;
	}
}
redirect ("main.php");
?>
