package com.fishjam.utility.image.tester;

import java.awt.image.BufferedImage;

import junit.framework.TestCase;

public class ImageTester  extends TestCase{
	public void testImage() {
		BufferedImage bufferedImage = new BufferedImage(100, 100, BufferedImage.TYPE_INT_ARGB);
		bufferedImage.getGraphics().fillRect(5, 5, 95, 95);
		
	}
}