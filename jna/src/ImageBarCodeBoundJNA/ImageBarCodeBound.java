package ImageBarCodeBoundJNA;

import com.google.gson.Gson;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.ptr.PointerByReference;

public class ImageBarCodeBound {
	// Alternative 1: interface-mapped class, dynamically load the C library
	public interface ImageBarCodeBoundLibrary extends Library {
		ImageBarCodeBoundLibrary INSTANCE = (ImageBarCodeBoundLibrary) Native.loadLibrary("ImageBarCodeBound",
				ImageBarCodeBoundLibrary.class);

		int bindImageAndBarCode(String strJSONIn, PointerByReference JSONOut);
		int freeJSONMemory(PointerByReference JSONOut);
	}

	//// Alternative 2: direct-mapped class (uses a concrete class rather than
	//// an
	//// interface, with a slight variation in method
	//// declarations).
	// public class CLibrary {
	// static {
	// Native.register("c");
	// }
	// }

	public static void main(String[] args) {
		String strTest = "{" +
				"    \"direction\": 0," +
				"    \"barcodes\": ["  +
				"        {"  +
				"            \"barcode\": \"1234567891测试\"," +
				"            \"abscissa\": 600," +
				"            \"ordinate\": 800" +
				"        }," +
				"        {" +
				"            \"barcode\": \"1234567892中文\"," +
				"            \"abscissa\": 700," +
				"            \"ordinate\": 900" +
				"        }" +
				"    ]," +
				"    \"images\": [" +
				"        {" +
				"            \"imageName\": \"aaa.png很好\"," +
				"            \"imagePath\": \"D:\\\\SinoCloud\\\\ImageBarCodeBound\\\\tsw_2016-12-08_072024929_23655.jpg\"" +
				//"        }," +
				//"        {" +
				//"            \"imageName\": \"bbb.png\"," +
				//"            \"imagePath\": \"D:/bbb.png\"" +
				"        }" +
				"    ]" +
				"}";
		System.out.println("json in: " + strTest);
//		Gson gson = new Gson();
//		BarcodeAndXrayForDeal jsonToPojo = gson.fromJson(strTest, BarcodeAndXrayForDeal.class);
//		String pojoToJson = gson.toJson(jsonToPojo);
//		System.out.println("jsonToPojo : " + pojoToJson);
		
		PointerByReference bufp = new PointerByReference();
		int nRet = ImageBarCodeBoundLibrary.INSTANCE.bindImageAndBarCode(strTest, bufp);
		System.out.println("bindImageAndBarCode return: " + nRet);
		if(nRet == 0) {
			String strResult = bufp.getPointer().getPointer(0).getString(0);
			ImageBarCodeBoundLibrary.INSTANCE.freeJSONMemory(bufp);
			System.out.println("json out: " + strResult);
			Gson gson = new Gson();
			ResultForDeal rfd = gson.fromJson(strResult, ResultForDeal.class);
			System.out.println("jsonToPojo : " +  gson.toJson(rfd));
			
		}
		
		System.out.println("job finished.");
	}
	
	static {		
		System.out.println("Working Directory = " + System.getProperty("user.dir"));
		System.setProperty("jna.debug_load", "true");
		System.setProperty("jna.debug_load.jna", "true");
//		System.setProperty("jna.library.path", "D:\\workspace\\ImageBarCodeBound\\native_x64");
		System.setProperty("jna.library.path", "native_x64");
		
//		System.out.println("jna.library.path: " + System.getProperty("jna.library.path"));
//		System.out.println("jna.platform.library.path: " + System.getProperty("jna.platform.library.path"));
//		System.out.println("java.library.path: " + System.getProperty("java.library.path"));
	}
}