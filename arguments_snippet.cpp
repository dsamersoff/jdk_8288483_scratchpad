jint Arguments::parse_java_options_environment_variable(ScopedVMInitArgs* args) {
  return parse_options_environment_variable("_JAVA_OPTIONS", args);
}

jint Arguments::parse_java_tool_options_environment_variable(ScopedVMInitArgs* args) {
  return parse_options_environment_variable("JAVA_TOOL_OPTIONS", args);
}

jint Arguments::parse_options_environment_variable(const char* name,
                                                   ScopedVMInitArgs* vm_args) {
  char *buffer = ::getenv(name);

  // Don't check this environment variable if user has special privileges
  // (e.g. unix su command).
  if (buffer == NULL || os::have_special_privileges()) {
    return JNI_OK;
  }

  if ((buffer = os::strdup(buffer)) == NULL) {
    return JNI_ENOMEM;
  }

  jio_fprintf(defaultStream::error_stream(),
              "Picked up %s: %s\n", name, buffer);

  int retcode = parse_options_buffer(name, buffer, strlen(buffer), vm_args);

  os::free(buffer);
  return retcode;
}

jint Arguments::parse_options_buffer(const char* name, char* buffer, const size_t buf_len, ScopedVMInitArgs* vm_args) {
  GrowableArray<JavaVMOption> *options = new (ResourceObj::C_HEAP, mtArguments) GrowableArray<JavaVMOption>(2, true);    // Construct option array

  // some pointers to help with parsing
  char *buffer_end = buffer + buf_len;
  char *opt_hd = buffer;
  char *wrt = buffer;
  char *rd = buffer;

  // parse all options
  while (rd < buffer_end) {
    // skip leading white space from the input string
    while (rd < buffer_end && isspace(*rd)) {
      rd++;
    }

    if (rd >= buffer_end) {
      break;
    }

    // Remember this is where we found the head of the token.
    opt_hd = wrt;

    // Tokens are strings of non white space characters separated
    // by one or more white spaces.
    while (rd < buffer_end && !isspace(*rd)) {
      if (*rd == '\'' || *rd == '"') {      // handle a quoted string
        int quote = *rd;                    // matching quote to look for
        rd++;                               // don't copy open quote
        while (rd < buffer_end && *rd != quote) {
                                            // include everything (even spaces)
                                            // up until the close quote
          *wrt++ = *rd++;                   // copy to option string
        }

        if (rd < buffer_end) {
          rd++;                             // don't copy close quote
        } else {
                                            // did not see closing quote
          jio_fprintf(defaultStream::error_stream(),
                      "Unmatched quote in %s\n", name);
          delete options;
          return JNI_ERR;
        }
      } else {
        *wrt++ = *rd++;                     // copy to option string
      }
    }

    // steal a white space character and set it to NULL
    *wrt++ = '\0';
    // We now have a complete token

    JavaVMOption option;
    option.optionString = opt_hd;
    option.extraInfo = NULL;

    options->append(option);                // Fill in option

    rd++;  // Advance to next character
  }

  // Fill out JavaVMInitArgs structure.
  jint status = vm_args->set_args(options);

  delete options;
  return status;
}
