# -*- mode: python -*-
a = Analysis([os.path.join(HOMEPATH,'support\\_mountzlib.py'), os.path.join(HOMEPATH,'support\\useUnicode.py'), 'C:\\Users\\om\\wikireader\\update-app\\WikiReader Update.py'],
             pathex=['C:\\pyinstaller-1.5.1'])

a.datas += [('7za.exe', 'C:\\Users\\om\\wikireader\\update-app\\tools\\7za.exe', 'DATA')]
pyz = PYZ(a.pure)
exe = EXE( pyz,
          a.scripts,
          a.binaries,
          a.zipfiles,
          a.datas,
          name=os.path.join('dist', 'WikiReader Update.exe'),
          debug=False,
          strip=False,
          upx=True,
          console=False , icon='C:\\Users\\om\\wikireader\\update-app\\images\\icon.ico')
app = BUNDLE(exe,
             name=os.path.join('dist', 'WikiReader Update.exe.app'))