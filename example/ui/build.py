"""把 src/ 下的 index.html + styles.css + app.js 打包成单文件 HTML。

用法： python build.py
输出： 自动热压机_上位机UI原型.html（可直接双击打开，无任何外部依赖）
"""
import pathlib

root = pathlib.Path(__file__).resolve().parent
src = root / "src"

html = (src / "index.html").read_text(encoding="utf-8")
css = (src / "styles.css").read_text(encoding="utf-8")
js = (src / "app.js").read_text(encoding="utf-8")

html = html.replace('<link rel="stylesheet" href="styles.css">', "<style>\n" + css + "\n</style>")
html = html.replace('<script src="app.js"></script>', "<script>\n" + js + "\n</script>")

out = root / "press_desktop.html"
out.write_text(html, encoding="utf-8")
print(f"built: {out.name}  ({len(html):,} chars)")
