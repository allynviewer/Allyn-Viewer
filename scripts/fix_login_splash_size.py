#!/usr/bin/env python3
from pathlib import Path

path = Path(r"d:\AllynViewer\indra\newview\llpanellogin.cpp")
text = path.read_text(encoding="utf-8")

start = text.find("void LLPanelLogin::reshapeBrowser()")
end = text.find("LLPanelLogin::~LLPanelLogin()", start)
if start < 0 or end < 0:
    raise SystemExit(f"markers not found start={start} end={end}")

new_fn = r'''void LLPanelLogin::reshapeBrowser()
{
	auto web_browser = getChild<LLMediaCtrl>("login_html");
	LLRect rect = gViewerWindow->getWindowRectScaled();
	mBrowserLayoutW = rect.getWidth();
	mBrowserLayoutH = rect.getHeight();

	LLRect html_rect;
	// -78 clears login chrome; -20 trims vertical overflow without shrinking page zoom.
	html_rect.setCenterAndSize(
	rect.getCenterX() /*- 2*/, rect.getCenterY() + 40,
	rect.getWidth() /*+ 6*/, rect.getHeight() - 98 );
	web_browser->setRect( html_rect );
	// Linden splash CSS uses max-width:1400 / max-height:850 centered on #282828.
	// Zoom by width only — using height in llmin() made -20px collapse the whole page.
	web_browser->setStretchToFill(true);
	web_browser->setMaintainAspectRatio(false);
	{
		const F32 design_w = 1400.f;
		F32 zoom = (F32)html_rect.getWidth() / design_w;
		// Re-apply every layout: CEF often ignores zoom set before the first navigate.
		web_browser->setPageZoomOverride(llmax(1.f, zoom));
	}
	web_browser->reshape( html_rect.getWidth(), html_rect.getHeight(), TRUE );
	reshape( rect.getWidth(), rect.getHeight(), 1 );
}

void LLPanelLogin::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPanel::reshape(width, height, called_from_parent);

	// Maximize / first-show WM_SIZE often arrives after the constructor layout.
	// Without this, the splash stays at the interim small size until next launch.
	if (!gViewerWindow)
		return;
	LLRect rect = gViewerWindow->getWindowRectScaled();
	if (rect.getWidth() == mBrowserLayoutW && rect.getHeight() == mBrowserLayoutH)
		return;

	static bool sInBrowserReshape = false;
	if (sInBrowserReshape)
		return;
	sInBrowserReshape = true;
	reshapeBrowser();
	sInBrowserReshape = false;
}

'''

text = text[:start] + new_fn + text[end:]

# Replace empty handleMediaEvent
old_media = """void LLPanelLogin::handleMediaEvent(LLPluginClassMedia* /*self*/, EMediaEvent event)
{
}"""
new_media = """void LLPanelLogin::handleMediaEvent(LLPluginClassMedia* /*self*/, EMediaEvent event)
{
	// CEF is ready / page finished — re-fit and re-zoom. First launch often
	// applied zoom before the plugin existed, so the splash stayed tiny.
	if (event == MEDIA_EVENT_NAVIGATE_COMPLETE || event == MEDIA_EVENT_SIZE_CHANGED)
	{
		reshapeBrowser();
	}
}"""

if old_media not in text:
    raise SystemExit("handleMediaEvent block not found")
text = text.replace(old_media, new_media, 1)

# In draw(), re-check window size in case reshape was skipped (LiruResizeRootWithScreen off)
old_draw = """void LLPanelLogin::draw()
{
	gGL.pushMatrix();"""
new_draw = """void LLPanelLogin::draw()
{
	if (gViewerWindow)
	{
		LLRect rect = gViewerWindow->getWindowRectScaled();
		if (rect.getWidth() != mBrowserLayoutW || rect.getHeight() != mBrowserLayoutH)
			reshapeBrowser();
	}

	gGL.pushMatrix();"""

if old_draw not in text:
    raise SystemExit("draw() block not found")
text = text.replace(old_draw, new_draw, 1)

path.write_text(text, encoding="utf-8", newline="\n")
print("llpanellogin.cpp updated OK")
