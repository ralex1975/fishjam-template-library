<%@page import="java.io.IOException"%>
<%@page import="javax.servlet.jsp.tagext.TryCatchFinally"%>
<%@ page language="java"
	import="java.util.*,javax.naming.*,java.sql.*,javax.sql.*"
	pageEncoding="UTF-8"%>
<%
String path = request.getContextPath();
String basePath = request.getScheme()+"://"+request.getServerName()+":"+request.getServerPort()+path+"/";
%>

<%!
	public void outputDBInfo(JspWriter out, Connection conn, String info) throws Exception
	{
			//获取Statement
			Statement stmt = conn.createStatement();
			//执行查询，返回ResulteSet对象
			ResultSet rs = stmt.executeQuery("select * from tlb_Students");
			out.println("Before output tlb_Students: " + info +"<br/>");
			while (rs.next()) {
				out.println(rs.getString(1) + "\t" + rs.getString(2)
						+ "<br/>");
			}
			conn.close();
			rs.close();
			stmt.close();

			out.println("After output tlb_Students: " + info +"<br/>");
	}

	public void testDBByContext(JspWriter out)  throws Exception{
			//初始化Context，使用InitialContext初始化Context
			Context ctx = new InitialContext();
			/*
			通过JNDI查找数据源，该JNDI为java:comp/env/jdbc/dstest，分成两个部分
			java:comp/env是Tomcat固定的，Tomcat提供的JNDI绑定都必须加该前缀
			jdbc/dstest是定义数据源时的数据源名
			 */
			DataSource ds = (DataSource) ctx.lookup("java:comp/env/"
					+ "jdbc/sqlitedemo");
			//获取数据库连接
			Connection conn = ds.getConnection();
			outputDBInfo(out, conn, "InitialContext");
	}
	
	public void testDBByClassFor(JspWriter out)  throws Exception
	{
			Class.forName("org.sqlite.JDBC");
			Connection conn = DriverManager
					.getConnection("jdbc:sqlite:F:/Fujie/FJCODE_GOOGLE/Web/JavaEEStudy/data/sqlitedemo.db");
		 
		 outputDBInfo(out, conn, "Class.forName");
	}
	%>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<base href="<%=basePath%>">

<title>My JSP 'AccessDB.jsp' starting page</title>

<meta http-equiv="pragma" content="no-cache">
<meta http-equiv="cache-control" content="no-cache">
<meta http-equiv="expires" content="0">
<meta http-equiv="keywords" content="keyword1,keyword2,keyword3">
<meta http-equiv="description" content="This is my page">
<!--
	<link rel="stylesheet" type="text/css" href="styles.css">
	-->

</head>

<body>
	<%
	testDBByContext(out);
	//testDBByClassFor(out);
%>
</body>
</html>
