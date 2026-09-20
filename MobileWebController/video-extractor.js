/**
 * OSSM Remote — video URL extractor (Cloudflare Worker)
 *
 * This tiny server fetches a video page on your behalf (bypassing CORS) and
 * returns the direct .mp4 / .m3u8 stream URL that the web app can play.
 *
 * Deploy (once):
 *   1. npm i -g wrangler   (or use: npx wrangler login)
 *   2. npx wrangler deploy video-extractor.js
 *   3. Copy the resulting URL, e.g. https://ossm-extractor.YOURNAME.workers.dev
 *      into the "Your extractor URL" field in Funscript mode (saved per device).
 *
 * Usage:
 *   https://<your-worker>.workers.dev/?url=<encoded page url>
 *
 * Response (JSON):
 *   { "url": "https://cdn.../video.mp4", "type": "mp4" }   or
 *   { "url": "https://cdn.../video.m3u8", "type": "hls" }  or
 *   { "error": "..." }
 */

const UA =
  "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0 Safari/537.36";

function json(obj, status) {
  return new Response(JSON.stringify(obj), {
    status,
    headers: {
      "Content-Type": "application/json; charset=utf-8",
      "Access-Control-Allow-Origin": "*",
      "Access-Control-Allow-Methods": "GET, HEAD, OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type",
    },
  });
}

function extractVideo(html) {
  // 0) Xvideos: html5player.setVideoUrlHigh('...mp4') / setVideoHLS('...m3u8')
  let m =
    html.match(/setVideoUrlHigh\s*\(\s*['"]([^'"]+\.mp4[^'"]*)['"]/i) ||
    html.match(/setVideoHLS\s*\(\s*['"]([^'"]+\.m3u8[^'"]*)['"]/i) ||
    html.match(/setVideoUrlLow\s*\(\s*['"]([^'"]+\.mp4[^'"]*)['"]/i);
  if (m) return { url: m[1], type: /m3u8/i.test(m[1]) ? "hls" : "mp4" };

  // Normalize JSON-escaped slashes (xhamster, Pornhub mediaDefinitions, …)
  const flat = html.replace(/\\\//g, "/");

  // 1) <source src / <video src>  (icegay.tv and similar)
  m =
    flat.match(/<source[^>]+src=["']([^"']+\.(?:mp4|m3u8)[^"']*)["']/i) ||
    flat.match(/<video[^>]+src=["']([^"']+\.(?:mp4|m3u8)[^"']*)["']/i);
  if (m) return { url: m[1], type: /m3u8/i.test(m[1]) ? "hls" : "mp4" };

  // 2) "videoUrl" / generic JSON stream keys (Pornhub, xhamster, …)
  m =
    flat.match(/"videoUrl"\s*:\s*"([^"]+\.mp4[^"]*)"/i) ||
    flat.match(/"(?:mp4|src|hls|video_url|videoHLS|streams)"\s*:\s*"([^"]+\.(?:mp4|m3u8)[^"]*)"/i);
  if (m) return { url: m[1], type: /m3u8/i.test(m[1]) ? "hls" : "mp4" };

  // 3) Any absolute mp4/m3u8 URL anywhere in the page
  m = flat.match(/https?:\/\/[^"'\\\s<>]+\.(?:mp4|m3u8)[^"'\\\s<>]*/i);
  if (m) return { url: m[0], type: /m3u8/i.test(m[0]) ? "hls" : "mp4" };

  return { error: "No direct video URL found in this page." };
}

export default {
  async fetch(request) {
    if (request.method === "OPTIONS") {
      return new Response(null, {
        status: 204,
        headers: {
          "Access-Control-Allow-Origin": "*",
          "Access-Control-Allow-Methods": "GET, HEAD, OPTIONS",
          "Access-Control-Allow-Headers": "Content-Type",
        },
      });
    }

    const target = new URL(request.url).searchParams.get("url");
    if (!target) return json({ error: "Missing ?url=" }, 400);
    if (!/^https?:\/\//i.test(target)) return json({ error: "Invalid URL" }, 400);

    try {
      const res = await fetch(target, {
        headers: {
          "User-Agent": UA,
          Accept: "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
        },
        redirect: "follow",
      });
      if (!res.ok) return json({ error: "Upstream status " + res.status }, 502);
      const html = await res.text();
      const out = extractVideo(html);
      if (out.error) return json(out, 404);
      return json(out, 200);
    } catch (e) {
      return json({ error: String((e && e.message) || e) }, 500);
    }
  },
};
