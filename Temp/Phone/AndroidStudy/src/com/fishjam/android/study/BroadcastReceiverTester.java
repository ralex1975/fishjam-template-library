package com.fishjam.android.study;
import android.test.ActivityUnitTestCase;;

/***************************************************************************************************************************************
* BroadcastReceiver -- ���չ㲥��Ϣ�����Խ����������ͨ�� sendBroadcast, sendStickyBroadcast, sendOrderedBroadcast �ȷ��͵���Ϣ��
*   onReceive -- ��д�÷���������ϵͳ�Ĺ㲥��Ϣ
*   
*   ע�᣺
*     1.AndroidManifest.xml �ļ���ʹ�� <receiver ... /> ���á�
*     2.������ͨ�� Context.registReceiver() ��ע��
***************************************************************************************************************************************/

public class BroadcastReceiverTester extends ActivityUnitTestCase{

	public BroadcastReceiverTester(Class activityClass) {
		super(activityClass);
		// TODO Auto-generated constructor stub
	}

}