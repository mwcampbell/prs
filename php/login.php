<?
require_once ("common.php");

parse_config_file ($station);

db_connect ();

if (!$username && !$user_username)
	html_error ("Login error.");
else if (!$user_username)
{
	$query = "select password, type from prs_user where user_name = '$username'";
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
	else if ($real_password != $password)
	{
		html_error ("Wrong password.");
	}
	else
	{
		$user_username = $username;
		$user_password = $real_password;
		$user_type = $real_type;
		session_register ("user_username", "user_password", "user_type");
		session_register ("station", "DB_HOST", "DB_NAME", "DB_USER", "DB_PASSWORD");
	}
}
redirect ("main.php");
?>
