"use strict";

function getCookie(cookieName) {
    var cookieString = document.cookie;

    var start = cookieString.indexOf(cookieName + '=');
    // 加上等号的原因是避免在某些 Cookie 的值里有与 cookieName 一样的字符串。
    if (start == -1) // 找不到
    {
        return null;
    }

    //获取到cookie值的开始位置
    start += cookieName.length + 1;
    var end = cookieString.indexOf(';', start);  //得到值的结束位置

    if (end == -1) {
        end = cookieString.length;　
    }

    return unescape(cookieString.substring(start, end));
}

//使用方法: setCookie('username','fishjam', 30)
function setCookie(c_name, value, expiredays) {
　  var exdate=new Date();
    exdate.setDate(exdate.getDate() + expiredays);
    document.cookie = c_name + "=" + escape(value) + ((expiredays==null) ? "" : ";expires="+exdate.toGMTString());
}

//用变量类型列表严格检测函数参数的辅助函数， 在用户函数入口调用该函数来检测每一个参数的类型，如果不满足，则抛出异常
//Release时改为 返回 bool ?
//使用示例： funciton myFunc( prefix, num, user) {
//      strictParams( [String, Number, Array ] , arguments );
//      后面进行函数的业务逻辑
//  }
function strictParams(types, args ){
	if( types.length != args.length ) {
	    //throw "Invalid number of arguments. Expected " + types.length + ", received " + args.length + " instead.";
	    alert("Invalid number of arguments. Expected " + types.length + ", received " + args.length + " instead.");
	    return;
	}
	for (var i = 0; i < args.length; i++) {
	    //console.log("%o -- %o", args[i].constructor, types[i]);
	    if (args[i].constructor != types[i]) {
	        //throw "Invalid argument type for " + i + ". Expected " + types[i].name + ", received " + args[i].constructor.name + " instead.";
	        alert("Invalid argument type for " + i + ". Expected " + types[i].name + ", received " + args[i].constructor.name + " instead.");
	        return;
		}
	}
}

function checkParams(funName, args, logType) {
    var argInfo = funName + ": "
    for (var i = 0; i < args.length; i++) {
        argInfo = argInfo + "[" + i + "]=" + typeof args[i] + ",";
    }
    if (logType != undefined) {
        switch(logType) {
        case 1:
            console.debug(argInfo);
        	break;
        case 2:
            console.info(argInfo);
        	break;
        case 3:
        default:
            console.log(argInfo);
            break;
        }
    }else{
        console.debug(argInfo);
    }
    return argInfo;
}


//通用的设置继承关系的方法?   -- Object-Oriented JavaScript.pdf 中介绍(P195)
function extend(Child, Parent) {
    var F = function() { };
    F.prototype = Parent.prototype;
    Child.prototype = new F();
    Child.prototype.constructor = Child;
    Child.uber = Parent.prototype;
}

String.prototype.format = function(args) {
    var result = this;
    if (arguments.length > 0) {    
        if (arguments.length == 1 && typeof (args) == "object") {
            for (var key in args) {
                if(args[key]!=undefined){
                    var reg = new RegExp("({" + key + "})", "g");
                    result = result.replace(reg, args[key]);
                }
            }
        }
        else {
            for (var i = 0; i < arguments.length; i++) {
                if (arguments[i] != undefined) {
                    var reg = new RegExp("({[" + i + "]})", "g");
                    result = result.replace(reg, arguments[i]);
                }
            }
        }
    }
    return result;
}

/*****************************************************************************************
* Url中可以输入 %\/?#&= 等特殊符号，如果直接在网络上传递或存入数据库，会因为乱码出错，
*   因此传输以前需要转换（TODO: 是否有标准的系统函数做这个工作?）
******************************************************************************************/
function handleUrlSpecialWord(quotReply) {
	if (null == quotReply) {
		return "";
	}
	
	return quotReply.replace(/%/g,"%25").replace(/\+/g,"%2B").replace(/\//g,"%2F")
			.replace(/\?/g,"%3F").replace(/#/g,"%23").replace(/&/g,"%26").replace(/=/g,"%3D");
}