using Newtonsoft.Json;
using System.Collections.Generic;

namespace golddrive
{
    [JsonObject(MemberSerialization.OptOut)]
    public class Settings
    {
        private string _args;

        public string Args
        {
            get { return _args?? (_args = ""); }
            set { _args = value; }
        }

        public Dictionary<string,Drive> Drives { get; set; }
    }
}
