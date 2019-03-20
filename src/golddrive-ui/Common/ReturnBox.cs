using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace golddrive_ui
{
    public class ReturnBox
    {
        public string Error { get; set; }
        public string Output { get; set; }
        public int ExitCode { get; set; }
        public bool Success { get; set; }

    }
}
