<?
if (!file_exists ("config.php")) {
?>

<HTML lang="en">
<HEAD>
<TITLE>PRS Manager: Welcome to PRS</TITLE>
</HEAD>

<BODY>
<H1>Welcome to PRS</H1>

<P>This is the web interface for PRS, a broadcast automation system.</P>

<P>You need to configure the web interface before you can use it. </P>

<P>Edit the file config.php.dist, giving the name and PRS config file path 
for each station you have.  Then save it as config.php and reload this page.</P>
</BODY>
</HTML>


<?
	exit;
}

require_once ("config.php");
require_once ("common.php");
html_start ("Log In");
?>
<p>Welcome to the PRS Manager. Please log in.</p>

<form name="login" action="login.php" method="post">
<div><label for="username">Username:</label>
<input type="text" name="username" id="username"><br>
<label for="password">Password:</label> <input type="password" name="password" id="password"><br>
<?
if ($STATIONS)
{
	echo ("<label for=\"station\">Station:</label>\n");
	echo ("<select id=\"station\" name=\"station_config\">\n");
	foreach ($STATIONS as $name => $config)
	{
		echo ("<option value=\"$config\">$name</option>\n");
	}
	echo ("</select><br>\n");
}
?>
<input type="submit" value="Log In"></div>
</form>
<?
html_end ();
?>
