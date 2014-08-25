using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Xml;
using Newtonsoft.Json;

namespace ProjectConverter {
    class Program {
        static void Main(string[] args) {
            if(args.Length != 3) {
                Console.WriteLine("Expecting 3 Arguments");
                Console.WriteLine("1: True/False To Convert XML To JSON");
                Console.WriteLine("2: Input File");
                Console.WriteLine("3: Output File");
                return;
            }
            bool toJSON;
            if(!bool.TryParse(args[0], out toJSON)) {
                Console.WriteLine("Could Not Parse First Argument (True/False)");
                return;
            }
            string fIn = args[1];
            string fOut = args[2];
            if(!File.Exists(fIn)) {
                Console.WriteLine("Could Not Find Input File");
                return;
            }
            if(fIn.Equals(fOut)) {
                Console.WriteLine("Can Not Output To Input File");
                return;
            }
            try {
                using(var s = File.Create(fOut)) {
                    Console.WriteLine("Beginning Write");
                    if(toJSON) ToJSON(fIn, s);
                    else ToXML(fIn, s);
                }
                Console.WriteLine("Conversion Complete");
            }
            catch(Exception) {
                Console.WriteLine("Could Not Create Output File");
                return;
            }
        }

        static void ToJSON(string fIn, FileStream fOut) {
            string xml, json;
            using(var s = File.OpenRead(fIn)) {
                xml = new StreamReader(s).ReadToEnd();
            }

            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            json = JsonConvert.SerializeXmlNode(doc, Newtonsoft.Json.Formatting.Indented);

            StreamWriter w = new StreamWriter(fOut);
            w.Write(json);
            w.Flush();
        }
        static void ToXML(string fIn, FileStream fOut) {
            string json;
            using(var s = File.OpenRead(fIn)) {
                json = new StreamReader(s).ReadToEnd();
            }

            XmlDocument doc = JsonConvert.DeserializeXmlNode(json);

            XmlWriterSettings xs = new XmlWriterSettings();
            xs.Indent = true;
            var xw = XmlWriter.Create(fOut, xs);
            doc.WriteTo(xw);
            xw.Flush();
        }
    }
}
