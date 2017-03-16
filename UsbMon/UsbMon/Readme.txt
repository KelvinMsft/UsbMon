///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
//

 Usb Hid Device Createion Process:
----------------------------------------------------
Driver Stack:

-----> MouHid / Kbdhid.sys			
			|
			|
			|
			|
			|	¡¡£ß£ß£ß£ß£ß£ß£ß HID Usb Device Stack			
			|	£¯				----------------	 £ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß 
			|	£ü				+	Client PDO +								               £Ü
			|	£ü				+--------------+	 £ß£ß£ß£ß£ß								    |
			|	£ü				+	  FDO	   +			¡¡£Ü			¡¡¡¡			            |
			|	£ü				----------------			   |								| Query Pnp Relations by FDO:
-----> UsbHid.sys(miniclass driver)							   |							   (7)		- Create Client Pdo	(0000000xx)	¡¡
			|       /\										   | AddDevice:						|							¡¡
			|	 ¡¡¡¡|										  (2)	- Create FDO(HID_xxxxxxx)	|							¡¡
			|¡¡£ß£ß  |									       |								|							¡¡
			|	|¡¡¡¡|     Call Mini-Class AddDevice		       |								|								
			|	|¡¡¡¡|£ß£ß£ß£ß£ß£ß£ß£ß(3)£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß / £ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£¯ £ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß    £ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß HidClass.sys(Class driver)							¡¡
			|	|											¡¡  /\			¡¡¡¡¡¡/\					¡¡¡¡/\					   ¡¡£Ü  
			|   |					¡¡¡¡							£ü				  |						|		                 |
			|   |					¡¡¡¡							£ü Actual		  | IRP Query Pnp		| IRP StartDevice Pnp	 |	¡¡
			|   |					¡¡¡¡							£ü AddDevice		 (6) Reations			(4) Reations             |	 
			|   |					¡¡¡¡						    (1)	Path		  |  					|                        |
			|   |					¡¡¡¡							£ü				  |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡   (5) IoInvalidateDeviceRelations
			|   |					¡¡¡¡							£ü				  |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡    |
			|   |					¡¡¡¡							£ü				  |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡    |
			|	|					¡¡¡¡							£ü				  |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡    |											
-----> UsbHub   |					¡¡¡¡							£ü				  |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡    |										
   (Bus Driver) |					¡¡¡¡							£ü				  |  					|¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡¡    |									¡¡¡¡ 
				£Ü£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£¯£ß£ß£ß£ß£ß£ß£ß£ß£ß£¯£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£¯£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß\/ £ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ß£ßPnP Manager
						UsbHid Expected AddDevice Call Path