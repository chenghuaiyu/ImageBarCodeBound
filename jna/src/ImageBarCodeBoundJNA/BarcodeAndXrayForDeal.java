package ImageBarCodeBoundJNA;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.List;

import com.yunkou.common.util.DateTimeUtil;

/**
 * @ClassName: BarcodeForDeal
 * @Description: 绠楁硶鐢ㄦ潯鐮乂O
 * @author-caihb
 * @date 2016骞�12鏈�21鏃� 涓嬪崍1:18:40
 *
 */
@SuppressWarnings("unused")
public class BarcodeAndXrayForDeal {

	private int direction;  //X鍏夋満杩愯鏂瑰悜
	private List<BarcodeForDeal> barcodes;
	private List<XrayForDeal> images;
	
	private class BarcodeForDeal {
		
		private String barcode;  //鍖呰９鍙�
		private int abscissa; //妯潗鏍�(鍨傜洿鐨甫杩愯鏂瑰悜 X)
		private int ordinate; //绾靛潗鏍�(娌跨潃鐨甫杩愯鏀捐 Y)
	}

	private class XrayForDeal {
		
		private String imageName;  //X鍏夊浘鐗囧悕绉�
		private String imagePath;  //X鍏夊浘鐗�
		
		public String getImageName() {
			return imageName;
		}
		public String getImagePath() {
			return imagePath;
		}
		public void setImagePath(String imagePath) {
			this.imagePath = imagePath;
		}
	}
}
