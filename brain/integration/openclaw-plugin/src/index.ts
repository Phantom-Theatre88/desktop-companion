import { randomUUID } from "node:crypto";
import { Type } from "typebox";
import { definePluginEntry } from "openclaw/plugin-sdk/plugin-entry";

const endpoint = "http://127.0.0.1:8765";

async function callApi(path: string, method: "GET" | "POST", body?: object) {
  const token = process.env.AKARI_API_TOKEN;
  if (!token) throw new Error("AKARI_API_TOKEN is not configured");
  const response = await fetch(endpoint + path, {
    method,
    headers: {"X-AKARI-Token": token, "Content-Type": "application/json"},
    ...(body ? {body: JSON.stringify(body)} : {}),
    signal: AbortSignal.timeout(15000),
  });
  const text = await response.text();
  if (!response.ok) throw new Error("AKARI HTTP " + response.status + ": " + text);
  return JSON.parse(text);
}

function toolResponse(data: unknown) {
  return {content: [{type: "text" as const, text: JSON.stringify(data)}]};
}

export default definePluginEntry({
  id: "akari-langgraph",
  name: "AKARI LangGraph Bridge",
  description: "Local asynchronous delegation to LangGraph",
  register(api) {
    api.registerTool({
      name: "akari_delegate",
      description: "Submit goal and retrieved memory context to LangGraph. The result is a job ID, NOT completion.",
      parameters: Type.Object({
        goal: Type.String(),
        context: Type.String({description: "Context retrieved from OpenClaw memory"}),
        constraints: Type.Array(Type.String()),
        job_id: Type.Optional(Type.String({description: "Reuse the original ID for retries"})),
      }),
      async execute(_id, params) {
        const job_id = params.job_id ?? randomUUID();
        return toolResponse(await callApi("/jobs", "POST", {
          job_id, goal: params.goal, context: params.context, constraints: params.constraints,
        }));
      },
    });
    api.registerTool({
      name: "akari_job_status",
      description: "Read status/result of delegated work without waiting for it in the conversation.",
      parameters: Type.Object({job_id: Type.String()}),
      async execute(_id, params) {
        return toolResponse(await callApi("/jobs/" + encodeURIComponent(params.job_id), "GET"));
      },
    });
    api.registerTool({
      name: "akari_resume",
      description: "Resume an interrupted job from its LangGraph checkpoint.",
      parameters: Type.Object({job_id: Type.String()}),
      async execute(_id, params) {
        return toolResponse(await callApi(
          "/jobs/" + encodeURIComponent(params.job_id) + "/resume", "POST",
        ));
      },
    });
  },
});
