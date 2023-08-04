/******************** (C) COPYRIGHT 2008 STMicroelectronics ********************
* File Name          : usb_desc.c
* Author             : MCD Application Team
* Version            : V2.2.1
* Date               : 09/22/2008
* Description        : Descriptors for Custom HID Demo
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"
#include "usb_desc.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* USB Standard Device Descriptor */
const u8 CustomHID_DeviceDescriptor[CUSTOMHID_SIZ_DEVICE_DESC] =
  {
    0x12,                       /*bLength */
    USB_DEVICE_DESCRIPTOR_TYPE, /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0x00,                       /*bDeviceClass*/
    0x00,                       /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    USB_FRAME_LEN,              /*bMaxPacketSize 40*/
    0x05,                       /*idVendor (0x0505)*/
    0x05,
    0x50,                       /*idProduct = 0x5050*/
    0x50,
    0x00,                       /*bcdDevice rel. 1.00*/
    0x02,
    1,                          /*Index of string descriptor describing
                                              manufacturer */
    2,                          /*Index of string descriptor describing
                                             product*/
    3,                          /*Index of string descriptor describing the
                                             device serial number */
    0x01                        /*bNumConfigurations*/
  }
  ; /* CustomHID_DeviceDescriptor */


/* USB Configuration Descriptor */
/*   All Descriptors (Configuration, Interface, Endpoint, Class, Vendor */
const u8 CustomHID_ConfigDescriptor[CUSTOMHID_SIZ_CONFIG_DESC] =
  {
    0x09, /* bLength: Configuation Descriptor size */
    USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType: Configuration */
    CUSTOMHID_SIZ_CONFIG_DESC,
    /* wTotalLength: Bytes returned */
    0x00,
    0x01,         /* bNumInterfaces: 1 interface */
    0x01,         /* bConfigurationValue: Configuration value */
    0x00,         /* iConfiguration: Index of string descriptor describing
                                 the configuration*/
    0xC0,         /* bmAttributes: Bus powered */
                  /*Bus powered: 7th bit, Self Powered: 6th bit, Remote wakeup: 5th bit, reserved: 4..0 bits */
    0x32,         /* MaxPower 100 mA: this current is used for detecting Vbus */
//    0x96,         /* MaxPower 300 mA: this current is used for detecting Vbus */
    /************** Descriptor of Custom HID interface ****************/
    /* 09 */
    0x09,         /* bLength: Interface Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,/* bDescriptorType: Interface descriptor type */
    0x00,         /* bInterfaceNumber: Number of Interface */
    0x00,         /* bAlternateSetting: Alternate setting */
    0x02,         /* bNumEndpoints */
    0x03,         /* bInterfaceClass: HID */
    0x00,         /* bInterfaceSubClass : 1=BOOT, 0=no boot */
    0x00,         /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
    0,            /* iInterface: Index of string descriptor */
    /******************** Descriptor of Custom HID HID ********************/
    /* 18 */
    0x09,         /* bLength: HID Descriptor size */
    HID_DESCRIPTOR_TYPE, /* bDescriptorType: HID */
    0x00,         /* bcdHID: HID Class Spec release number */
    0x01,
    0x00,         /* bCountryCode: Hardware target country */
    0x01,         /* bNumDescriptors: Number of HID class descriptors to follow */
    0x22,         /* bDescriptorType */
    CUSTOMHID_SIZ_REPORT_DESC,/* wItemLength: Total length of Report descriptor */
    0x00,
    /******************** Descriptor of Custom HID endpoints ******************/
    /* 27 */
    0x07,          /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE, /* bDescriptorType: */

    0x82,          /* bEndpointAddress: Endpoint Address (IN) */               
    //�˵�2 ����               // bit 3...0 : the endpoint number
    //��PC������               // bit 6...4 : reserved
                    // bit 7     : 0(OUT), 1(IN)
    0x03,          /* bmAttributes: Interrupt endpoint */
    USB_FRAME_LEN,  /* wMaxPacketSize:SendLength Bytes max *///��PC����SendLength�ֽ�����
    0x00,
    0x0A,          /* bInterval: Polling Interval (16ms) */
    /* 34 */
    	
    0x07,	/* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,	/* bDescriptorType: */
			/*	Endpoint descriptor type */
    0x01,	/* bEndpointAddress: */
	//�˵�1 ���  		/*	Endpoint Address (OUT) */
	//����PC��Ƭ�����͵�����
	0x03,	/* bmAttributes: Interrupt endpoint */
    USB_FRAME_LEN,/* wMaxPacketSize: ReceiveLength Bytes max  */
    0x00,//����pc������������ ReceiveLength�� �ֽ�
    0x0A,	/* bInterval: Polling Interval (16 ms) */
    /* 41 */
  }
  ; /* CustomHID_ConfigDescriptor */
const u8 CustomHID_ReportDescriptor[CUSTOMHID_SIZ_REPORT_DESC] = 
{ 
#if 1 
0x05, 0x8c, /* USAGE_PAGE (ST Page) */ 
0x09, 0x01, /* USAGE (Demo Kit) */ 
0xa1, 0x01, /* COLLECTION (Application) */ 
/* 6 */ 

// The Input report 
0x09,0x03, // USAGE ID - Vendor defined 
0x15,0x00, // LOGICAL_MINIMUM (0) 
0x25,0xFF, // LOGICAL_MAXIMUM (255) 
0x75,0x08, // REPORT_SIZE (8) 
0x95,USB_FRAME_LEN, // REPORT_COUNT :SendLength 
0x81,0x02, // INPUT (Data,Var,Abs) 
//19
// The Output report 
0x09,0x04, // USAGE ID - Vendor defined 
0x15,0x00, // LOGICAL_MINIMUM (0) 
0x25,0xFF, // LOGICAL_MAXIMUM (255) 
0x75,0x08, // REPORT_SIZE (8) 
0x95,USB_FRAME_LEN, // REPORT_COUNT:ReceiveLength 
0x91,0x02, // OUTPUT (Data,Var,Abs) 
//32
// The Feature report

0x09, 0x05, // USAGE ID - Vendor defined 
0x15,0x00, // LOGICAL_MINIMUM (0) 
0x25,0xFF, // LOGICAL_MAXIMUM (255) 
0x75,0x08, // REPORT_SIZE (8) 
0x95,0x02, // REPORT_COUNT (2) 
0xB1,0x02, 

/* 45 */ 
0xc0 /* END_COLLECTION */ 
#endif 

#if 0
    //��ʾ��;ҳΪͨ�������豸
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    //��ʾ��;Ϊ����
    0x09, 0x06,                    // USAGE (Keyboard)
    
    //��ʾӦ�ü��ϣ�����Ҫ��END_COLLECTION����������������END_COLLECTION
    0xa1, 0x01,                    // COLLECTION (Application)
    
    //�������⼸��������һ�������õ��ֶΣ��ܹ�Ϊ8��bits��ÿ��bit��ʾһ������
    //�ֱ����ctrl������GUI������8��bits�պù���һ���ֽڣ���λ�ڱ���ĵ�һ���ֽڡ�
    //�������λ����bit-0��Ӧ����ctrl����������ص����ݸ�λΪ1�����ʾ��ctrl�������£�
    //������ctrl��û�а��¡����λ����bit-7��ʾ��GUI���İ���������м�ļ���λ��
    //��Ҫ����HIDЭ���й涨����;ҳ��HID Usage Tables����ȷ��������ͨ��������ʾ
    //�����������ctrl��shift��del����
    //��ʾ��;ҳΪ����
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    //�����С��������ֶεĿ�ȣ�Ϊ1bit������ǰ����߼���СֵΪ0���߼����ֵΪ1
    0x75, 0x01,                    //   REPORT_SIZE (1)
    //����ĸ���Ϊ8�����ܹ���8��bits
    0x95, 0x08,                    //   REPORT_COUNT (8)
    //��;��Сֵ������Ϊ��ctrl��
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    //��;���ֵ������Ϊ��GUI������window��
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    //�߼���СֵΪ0
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    //�߼����ֵΪ1
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    //�����ã�������ֵ������ֵ�����������һ�㱨�����ֵ��
    //������ƶ��������򱨸����ֵ����ʾ����ƶ�����
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    
 
    //���������ݶθ���Ϊ1
    //��8��bit�ǳ������豸���뷵��0
    0x95, 0x01,                    //   REPORT_COUNT (1)
    //ÿ���γ���Ϊ8bits
    0x75, 0x08,                    //   REPORT_SIZE (8)
    //�����ã�������ֵ������ֵ
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    
    //���¶�����6��8bit������飬ÿ��8bit����һ���ֽڣ�������ʾһ�����������Կ���ͬʱ
    //��6���������¡�û�а�������ʱ��ȫ������0��������µļ�̫�࣬���¼���ɨ��ϵͳ
    //�޷����ְ���ʱ����ȫ������0x01����6��0x01�������һ�������£�����6���ֽ��еĵ�һ
    //���ֽ�Ϊ��Ӧ�ļ�ֵ�������ֵ�ο�HID Usage Tables����������������£����1��2����
    //�ֽڷֱ�Ϊ��Ӧ�ļ�ֵ���Դ����ơ�
    //�������Ϊ6
    0x95, 0x3E,                    //   REPORT_COUNT (6)
    //ÿ���δ�СΪ8bits
    0x75, 0x08,                    //   REPORT_SIZE (8)
    //ʹ����СֵΪ0
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))
    //ʹ�����ֵΪ0x65
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)
    //�߼���Сֵ0
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    //�߼����ֵ255
    0x25, 0xFF,                    //   LOGICAL_MAXIMUM (255)
    //�����ã����������飬����ֵ
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
       
    //��;��LED�����������Ƽ����ϵ�LED�õģ���������˵�����������
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    //���������ݶθ���Ϊ5
    0x95, 0x05,                    //   REPORT_COUNT (5)
    //ÿ���δ�СΪ1bit
    0x75, 0x01,                    //   REPORT_SIZE (1)
    //��;��Сֵ��Num Lock�������ּ�������
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    //��;���ֵ��Kana�������ʲô����Ҳ�����^_^
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    //��ǰ����˵������ֶ�������õģ���������LED��������ֵ������ֵ��
    //1��ʾ������0��ʾ����
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    //���������ݶθ���Ϊ1
    0x95, 0x01,                    //   REPORT_COUNT (1)
    //ÿ���δ�СΪ3bits
    0x75, 0x03,                    //   REPORT_SIZE (3)
    //����ã�������ֵ������
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)    
    //����Ҫ���ֽڶ��룬��ǰ�����LED��ֻ����5��bit��
    //���Ժ�����Ҫ����3������bit������Ϊ������
    
    0x09, 0x05, // USAGE ID - Vendor defined 
    0x15, 0x00, // LOGICAL_MINIMUM (0) 
    0x25, 0xFF, // LOGICAL_MAXIMUM (255) 
    0x75, 0x08, // REPORT_SIZE (8) 
    0x95, 0x02, // REPORT_COUNT (2) 
    0xB1, 0x02, 

    //�ؼ��ϣ�������Ķ�Ӧ
    0xc0                           // END_COLLECTION
#endif


#if 0
0x05,0x01,
0x09,0x06,
0xA1,0x01,

0x05,0x07,
0x19,0x00,
0x29,0xff,
0x15,0x00,
0x25,0xff,
0x75,0x01,
0x95,0x08,
0x91,0x02,

0x05,0x07,
0x19,0xE0,
0x29,0xE7,
0x95,0x08,
0x81,0x02,

0x75,0x08,
0x95,0x01,
0x81,0x01,

0x19,0x00,
0x29,0x93,
0x26,0xFF,0x00,
0x95,0x3E,
0x81,0x00,
0xC0

#endif

}; /* CustomHID_ReportDescriptor */ 


/* USB String Descriptors (optional) */
const u8 CustomHID_StringLangID[CUSTOMHID_SIZ_STRING_LANGID] =
  {
    CUSTOMHID_SIZ_STRING_LANGID,
    USB_STRING_DESCRIPTOR_TYPE,
    0x09,
    0x04
  }
  ; /* LangID = 0x0409: U.S. English */

const u8 CustomHID_StringVendor[CUSTOMHID_SIZ_STRING_VENDOR] =
  {
    CUSTOMHID_SIZ_STRING_VENDOR, /* Size of Vendor string */
    USB_STRING_DESCRIPTOR_TYPE,  /* bDescriptorType*/
    'A', 0, 
    'n', 0, 
    'y', 0,
    'I', 0,
    'D', 0, 
    ' ', 0, 
    'H', 0,
    'I', 0,
    'D', 0
  };

const u8 CustomHID_StringProduct[CUSTOMHID_SIZ_STRING_PRODUCT] =
{
    CUSTOMHID_SIZ_STRING_PRODUCT,           /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,             /* bDescriptorType */
    'A', 0, 
    'n', 0, 
    'y', 0, 
    'I', 0, 
    'D', 0, 
    ' ', 0,
    'R', 0,
    'e', 0,
    'a', 0,
    'd', 0,
    'e', 0,
    'r', 0
};
u8 CustomHID_StringSerial[CUSTOMHID_SIZ_STRING_SERIAL] =
  {
    CUSTOMHID_SIZ_STRING_SERIAL,        /* bLength */
    USB_STRING_DESCRIPTOR_TYPE,         /* bDescriptorType */
    'R', 0, 
    '3', 0, 
    '2', 0,
    '1', 0
  };

/******************* (C) COPYRIGHT 2008 STMicroelectronics *****END OF FILE****/

