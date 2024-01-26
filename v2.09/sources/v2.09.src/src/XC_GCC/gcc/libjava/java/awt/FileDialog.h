
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_FileDialog__
#define __java_awt_FileDialog__

#pragma interface

#include <java/awt/Dialog.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class Dialog;
        class FileDialog;
        class Frame;
    }
  }
}

class java::awt::FileDialog : public ::java::awt::Dialog
{

public:
  FileDialog(::java::awt::Dialog *);
  FileDialog(::java::awt::Dialog *, ::java::lang::String *);
  FileDialog(::java::awt::Dialog *, ::java::lang::String *, jint);
  FileDialog(::java::awt::Frame *);
  FileDialog(::java::awt::Frame *, ::java::lang::String *);
  FileDialog(::java::awt::Frame *, ::java::lang::String *, jint);
  virtual jint getMode();
  virtual void setMode(jint);
  virtual ::java::lang::String * getDirectory();
  virtual void setDirectory(::java::lang::String *);
  virtual ::java::lang::String * getFile();
  virtual void setFile(::java::lang::String *);
  virtual ::java::io::FilenameFilter * getFilenameFilter();
  virtual void setFilenameFilter(::java::io::FilenameFilter *);
  virtual void addNotify();
public: // actually protected
  virtual ::java::lang::String * paramString();
public: // actually package-private
  virtual ::java::lang::String * generateName();
private:
  static jlong getUniqueLong();
public:
  static const jint LOAD = 0;
  static const jint SAVE = 1;
private:
  static const jlong serialVersionUID = 5035145889651310422LL;
  ::java::lang::String * __attribute__((aligned(__alignof__( ::java::awt::Dialog)))) dir;
  ::java::lang::String * file;
  ::java::io::FilenameFilter * filter;
  jint mode;
  static jlong next_file_dialog_number;
public:
  static ::java::lang::Class class$;
};

#endif // __java_awt_FileDialog__