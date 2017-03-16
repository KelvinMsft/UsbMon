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
			|				   HID Usb Device Stack			
			|					----------------				 
			|					+	Client PDO +	<-------------------------------------------\
			|					+--------------+												|
			|					+	  FDO	   +	<----------\								|
			|					----------------			   |								| Query Pnp Relations by FDO:
-----> UsbHid.sys(miniclass driver)							   |							   (7)		- Create Client Pdo	(0000000xx)	��
			|  ---  /\										   | AddDevice:						|							��
			|	|����|										  (2)	- Create FDO(HID_xxxxxxx)	|							��
			|	|����|									       |								|							��
			|	|����|									       |								|								
			|	|����|�ߣߣߣߣߣߣߣ�(3)�ߣߣߣߣߣߣߣߣߣߣߣ� / HidClass.sys(class driver)�ߣߣ�/�ߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣ� 								��
			|	|											��  /\				 /\						 /\					   �� \
			|   |					����							��				  |						  |						��|
			|   |					����							�� Actual		  | IRP Query Pnp		  | IRP StartDevice Pnp	 (5)	��
			|   |					����							�� AddDevice		 (6) Reations			 (4) Reations			��|	IoInvalidateDeviceRelations
			|   |					����						    (1)				  |  					  |  					��|
			|   |					����							��				  |  					  |  				�� �� \/
			|	|�ߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣ�/�ߣߣߣߣߣߣߣߣ�/�ߣߣߣߣߣߣߣߣߣߣߣ� /	�ߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣߣ�PnP Manager 	
			|				Mini Driver Expected Path			 						
			|
-----> UsbHub.sys											
														���� 