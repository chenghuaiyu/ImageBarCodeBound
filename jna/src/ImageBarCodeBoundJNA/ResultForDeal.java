package ImageBarCodeBoundJNA;

import java.util.ArrayList;
import java.util.List;

/**
 * @ClassName: ResultForDeal
 * @Description: 绠楁硶杩斿洖VO
 * @author-caihb
 * @date 2016骞�12鏈�21鏃� 涓嬪崍1:18:40
 *
 */
@SuppressWarnings("unused")
public class ResultForDeal {

	private String status;
	private List<Barcode> barcodes = new ArrayList<Barcode>();
	
	
	private class Barcode {
		private String barcode;
		private String imageName;
		private String imagePath;
		
		public String getImagePath() {
			return imagePath;
		}
		public void setImagePath(String imagePath) {
			this.imagePath = imagePath;
		}
	}
}
